#include <fstream>
#include <iostream>
#include <memory>

#include <SDL.h>

#include <libChip-8/include/chip8vm.hpp>


using namespace std;

// Screen dimensions.
const int SCALING = 16;
const int SCREEN_WIDTH = Chip8VM::SCREEN_WIDTH * SCALING;
const int SCREEN_HEIGHT = Chip8VM::SCREEN_HEIGHT * SCALING;

enum { SUCCEEDED, FAILED, USAGE };

// A unique pointer with a custom destructor, used as a handle for SDL objects.
template<typename C>
using handle = std::unique_ptr<C, void(*)(C*)>;


void load_rom(Chip8VM& vm, string filename)
{
	// Load the ROM file into a buffer.
	ifstream is(filename, ifstream::binary | ios::ate);
	size_t len = static_cast<size_t>(is.tellg());
	auto buffer = make_unique<Chip8VM::Byte[]>(len);
	is.seekg(0, is.beg);
	is.read((char*)buffer.get(), len);
	is.close();

	// Supply the buffer to the Chip-8 VM.
	vm.load(buffer.get(), len);
}


Chip8VM::Key convert_scancode(SDL_Scancode scancode)
{
	switch (scancode)
	{
	case SDL_SCANCODE_1:
		return Chip8VM::Key::KEY_1;
	case SDL_SCANCODE_2:
		return Chip8VM::Key::KEY_2;
	case SDL_SCANCODE_3:
		return Chip8VM::Key::KEY_3;
	case SDL_SCANCODE_4:
		return Chip8VM::Key::KEY_C;
	case SDL_SCANCODE_Q:
		return Chip8VM::Key::KEY_4;
	case SDL_SCANCODE_W:
		return Chip8VM::Key::KEY_5;
	case SDL_SCANCODE_E:
		return Chip8VM::Key::KEY_6;
	case SDL_SCANCODE_R:
		return Chip8VM::Key::KEY_D;
	case SDL_SCANCODE_A:
		return Chip8VM::Key::KEY_7;
	case SDL_SCANCODE_S:
		return Chip8VM::Key::KEY_8;
	case SDL_SCANCODE_D:
		return Chip8VM::Key::KEY_9;
	case SDL_SCANCODE_F:
		return Chip8VM::Key::KEY_E;
	case SDL_SCANCODE_Z:
		return Chip8VM::Key::KEY_A;
	case SDL_SCANCODE_X:
		return Chip8VM::Key::KEY_0;
	case SDL_SCANCODE_C:
		return Chip8VM::Key::KEY_B;
	case SDL_SCANCODE_V:
		return Chip8VM::Key::KEY_F;
	default:
		return Chip8VM::Key::NO_KEY;
	}
}


int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cerr << "Usage: " << argv[0] << " <filename>\n";
		return USAGE;
	}

	auto vm_handle = make_unique<Chip8VM>();
	Chip8VM& vm = *vm_handle;

	load_rom(vm, argv[1]);

	// Initialise SDL.
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		cerr << "SDL_Init: Error: " << SDL_GetError() << endl;
		return FAILED;
	}

	// Create an SDL window, wrapping it in a handle to clear it up automatically.
	handle<SDL_Window> window_handle{
		SDL_CreateWindow("CHIP-8 in C++", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN),
		SDL_DestroyWindow };
	SDL_Window* window = window_handle.get();

	if (window == nullptr)
	{
		cerr << "SDL_CreateWindow: Error: " << SDL_GetError() << endl;
		SDL_Quit();
		return FAILED;
	}

	// Create an SDL renderer, wrapping it in a handle to clear it up automatically.
	handle<SDL_Renderer> rh(
		SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
		SDL_DestroyRenderer);
	SDL_Renderer* renderer = rh.get();

	if (renderer == nullptr)
	{
		cerr << "SDL_CreateRenderer: Error: " << SDL_GetError() << endl;
		SDL_Quit();
		return FAILED;
	}

	SDL_Event event;
	bool quit = false;
	while (!quit)
	{
		// Process events.
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				vm.key_pressed(convert_scancode(event.key.keysym.scancode));
				break;
			case SDL_KEYUP:
				vm.key_released(convert_scancode(event.key.keysym.scancode));
				break;
			}
		}

		// Tick the delay timer (based on the not necessarily true assumption that we're refreshing at 60Hz).
		vm.tick();

		// Bump the VM on by a few instructions.
		vm.step(10);

		// Clear the screen in dark grey.
		SDL_SetRenderDrawColor(renderer, 0x0f, 0x0f, 0x0f, 0xff);
		SDL_RenderClear(renderer);

		// Draw the VM's screen in green.
		SDL_SetRenderDrawColor(renderer, 0x00, 0xff, 0x00, 0xff);
		for (auto y = 0; y < Chip8VM::SCREEN_HEIGHT; y++)
		{
			for (auto x = 0; x < Chip8VM::SCREEN_WIDTH; x++)
			{
				if (vm.io.screen.test(x + Chip8VM::SCREEN_WIDTH * y))
				{
					SDL_Rect rect{ x * SCALING, y * SCALING, SCALING, SCALING };
					SDL_RenderFillRect(renderer, &rect);
				}
			}
		}

		SDL_RenderPresent(renderer);
	}

	// Clean up and quit.
	SDL_Quit();

	return SUCCEEDED;
}
