#include "chip8vm.hpp"


// Fonts (source: https://github.com/DanTup/DaChip8/blob/master/DaChip8/Font.cs)
static uint8_t font[] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,	// 0
	0x20, 0x60, 0x20, 0x20, 0x70,	// 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0,	// 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0,	// 3
	0x90, 0x90, 0xF0, 0x10, 0x10,	// 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0,	// 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0,	// 6
	0xF0, 0x10, 0x20, 0x40, 0x40,	// 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0,	// 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0,	// 9
	0xF0, 0x90, 0xF0, 0x90, 0x90,	// A
	0xE0, 0x90, 0xE0, 0x90, 0xE0,	// B
	0xF0, 0x80, 0x80, 0x80, 0xF0,	// C
	0xE0, 0x90, 0x90, 0x90, 0xE0,	// D
	0xF0, 0x80, 0xF0, 0x80, 0xF0,	// E
	0xF0, 0x80, 0xF0, 0x80, 0x80	// F
};


// The VM's constructor.
Chip8VM::Chip8VM()
{
	copy(&font[0], &font[sizeof(font)], memory.begin());
	reset();
}


// Resets the VM.
void Chip8VM::reset()
{
	shadow.fill(&Chip8VM::i_illegal);
	io.screen.reset();
	io.keys.fill(false);
	reg.pc = 0x200;
	here = 0x200;
	reg.sp = 0;
	is_blocked = false;
	key = Key::NO_KEY;
	random_number_engine.seed(random_device{}());
}


// Returns a random byte from 0 to 255 inclusive.
Chip8VM::Byte Chip8VM::rnd()
{
	return uniform_int_distribution<int>(0, 255)(random_number_engine);
}


// Pushes an address onto the stack.
void Chip8VM::push(Address address)
{
	reg.stack[reg.sp++] = address;
	if (reg.sp == reg.stack.size())
	{
		reg.sp = 0;
	}
}


// Pops an address from the stack.
Chip8VM::Address Chip8VM::pop()
{
	if (reg.sp == 0)
	{
		reg.sp = (Byte)reg.stack.size();
	}
	return reg.stack[--reg.sp];
}


// Writes an opcode into two consecutive bytes of VM memory at the 'here' pointer.
void Chip8VM::write_ram(Opcode opcode)
{
	memory[here] = opcode >> 8;
	memory[here + 1] = opcode & 0xff;
}


// Writes an instruction into shadow memory at the 'here' pointer.
void Chip8VM::write_shadow(Instruction i)
{
	shadow[here] = i;
}


// Returns an instruction given an opcode.
Chip8VM::Instruction Chip8VM::instruction_from_opcode(Opcode opcode)
{
	if ((opcode & 0xfff0) == 0x00e0)
	{
		switch (opcode & 0x0f)
		{
		case 0x0:
			return &Chip8VM::i_cls;
		case 0xe:
			return &Chip8VM::i_ret;
		}
	}
	else if ((opcode & 0xf000) == 0x1000)
	{
		return &Chip8VM::i_jp;
	}
	else if ((opcode & 0xf000) == 0x2000)
	{
		return &Chip8VM::i_call;
	}
	else if ((opcode & 0xf000) == 0x3000)
	{
		return &Chip8VM::i_se_vx_imm;
	}
	else if ((opcode & 0xf000) == 0x4000)
	{
		return &Chip8VM::i_sne_vx_imm;
	}
	else if ((opcode & 0xf00f) == 0x5000)
	{
		return &Chip8VM::i_se_vx_vy;
	}
	else if ((opcode & 0xf000) == 0x6000)
	{
		return &Chip8VM::i_ld_vx_imm;
	}
	else if ((opcode & 0xf000) == 0x7000)
	{
		return &Chip8VM::i_add_vx_imm;
	}
	else if ((opcode & 0xf000) == 0x8000)
	{
		switch (opcode & 0xf)
		{
		case 0x0:
			return &Chip8VM::i_ld_vx_vy;
		case 0x1:
			return &Chip8VM::i_or_vx_vy;
		case 0x2:
			return &Chip8VM::i_and_vx_vy;
		case 0x3:
			return &Chip8VM::i_xor_vx_vy;
		case 0x4:
			return &Chip8VM::i_add_vx_vy;
		case 0x5:
			return &Chip8VM::i_sub_vx_vy;
		case 0x6:
			return &Chip8VM::i_ld_vx_shr_vy;
		case 0x7:
			return &Chip8VM::i_subn_vx_vy;
		case 0xe:
			return &Chip8VM::i_ld_vx_shl_vy;
		}
	}
	else if ((opcode & 0xf00f) == 0x9000)
	{
		return &Chip8VM::i_sne_vx_vy;
	}
	else if ((opcode & 0xf000) == 0xa000)
	{
		return &Chip8VM::i_ld_i_addr;
	}
	else if ((opcode & 0xf000) == 0xb000)
	{
		return &Chip8VM::i_jp_v0;
	}
	else if ((opcode & 0xf000) == 0xc000)
	{
		return &Chip8VM::i_rnd_vx_imm;
	}
	else if ((opcode & 0xf000) == 0xd000)
	{
		return &Chip8VM::i_drw_vx_vy_n;
	}
	else if ((opcode & 0xf0ff) == 0xe09e)
	{
		return &Chip8VM::i_skp_vx;
	}
	else if ((opcode & 0xf0ff) == 0xe0a1)
	{
		return &Chip8VM::i_sknp_vx;
	}
	else if ((opcode & 0xf000) == 0xf000)
	{
		switch (opcode & 0xff)
		{
		case 0x07:
			return &Chip8VM::i_ld_vx_dt;
		case 0x0a:
			return &Chip8VM::i_ld_vx_k;
		case 0x15:
			return &Chip8VM::i_ld_dt_vx;
		case 0x18:
			return &Chip8VM::i_ld_st_vx;
		case 0x1e:
			return &Chip8VM::i_add_i_vx;
		case 0x29:
			return &Chip8VM::i_ld_f_vx;
		case 0x33:
			return &Chip8VM::i_ld_b_vx;
		case 0x55:
			return &Chip8VM::i_ld_i_vx;
		case 0x65:
			return &Chip8VM::i_ld_vx_i;
		}
	}
	return &Chip8VM::i_illegal;
}


// Loads a program into VM memory and compiles it.
void Chip8VM::load(Byte* data, size_t len)
{
	reset();
	for (size_t i = 0; i < len; i += 2)
	{
		Opcode opcode = (data[i] << 8) | data[i + 1];
		compile(opcode);
	}
}


// Compiles an opcode into VM memory at the 'here' pointer.
void Chip8VM::compile(Opcode opcode)
{
	write_ram(opcode);
	auto instruction = instruction_from_opcode(opcode);
	write_shadow(instruction);
	here += 2;
}


// Executes an illegal instruction as a NOP (no operation).
void Chip8VM::i_illegal()
{
	reg.pc += 2;
}


// Clears the screen.
void Chip8VM::i_cls()
{
	io.screen.reset();
	reg.pc += 2;
}


// Returns from a subroutine.
void Chip8VM::i_ret()
{
	reg.pc = pop();
}


// Jumps to an address.
void Chip8VM::i_jp()
{
	Address target = ((memory[reg.pc] & 0x0f) << 8) | memory[reg.pc + 1];
	reg.pc = target;
}


// Calls a subroutine.
void Chip8VM::i_call()
{
	Address target = ((memory[reg.pc] & 0x0f) << 8) | memory[reg.pc + 1];
	push(reg.pc + 2);
	reg.pc = target;
}


// Skips the next instruction if register Vx equals an immediate value.
void Chip8VM::i_se_vx_imm()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte byte = memory[reg.pc + 1];
	reg.pc += (reg.v[vx] == byte) ? 4 : 2;
}


// Skips the next instruction if register Vx does not equal an immediate value.
void Chip8VM::i_sne_vx_imm()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte byte = memory[reg.pc + 1];
	reg.pc += (reg.v[vx] != byte) ? 4 : 2;
}


// Skips the next instruction if register Vx equals register Vy.
void Chip8VM::i_se_vx_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vy = memory[reg.pc + 1] >> 4;
	reg.pc += (reg.v[vx] == reg.v[vy]) ? 4 : 2;
}


// Loads register Vx with an immediate value.
void Chip8VM::i_ld_vx_imm()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte byte = memory[reg.pc + 1];
	reg.v[vx] = byte;
	reg.pc += 2;
}


// Adds an immediate value to register Vx.
void Chip8VM::i_add_vx_imm()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte byte = memory[reg.pc + 1];
	reg.v[vx] += byte;
	reg.pc += 2;
}


// Loads register Vx with register Vy.
void Chip8VM::i_ld_vx_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vy = memory[reg.pc + 1] >> 4;
	reg.v[vx] = reg.v[vy];
	reg.pc += 2;
}


// ORs register Vx with register Vy leaving the result in Vx.
void Chip8VM::i_or_vx_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vy = memory[reg.pc + 1] >> 4;
	reg.v[vx] |= reg.v[vy];
	reg.pc += 2;
}


// ANDs register Vx with register Vy leaving the result in Vx.
void Chip8VM::i_and_vx_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vy = memory[reg.pc + 1] >> 4;
	reg.v[vx] &= reg.v[vy];
	reg.pc += 2;
}


// XORs register Vx with register Vy leaving the result in Vx.
void Chip8VM::i_xor_vx_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vy = memory[reg.pc + 1] >> 4;
	reg.v[vx] ^= reg.v[vy];
	reg.pc += 2;
}


// Adds register Vy to register Vx leaving the result in Vx. TODO: explain how this affects the flags.
void Chip8VM::i_add_vx_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vy = memory[reg.pc + 1] >> 4;
	uint16_t result = reg.v[vx] + reg.v[vy];
	reg.v[vx] = result & 0xff;
	reg.v[0x0f] = (result & 0xff00) ? 1 : 0;
	reg.pc += 2;
}


// Subtracts register Vy from register Vx leaving the result in Vx. TODO: explain how this affects the flags.
void Chip8VM::i_sub_vx_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vy = memory[reg.pc + 1] >> 4;
	Byte vf = reg.v[vx] > reg.v[vy] ? 1 : 0;
	uint16_t result = reg.v[vx] - reg.v[vy];
	reg.v[vx] = result & 0xff;
	reg.v[0x0f] = vf;
	reg.pc += 2;
}


// Shifts register Vx right. TODO: explain how this affects the flags.
void Chip8VM::i_ld_vx_shr_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vf = reg.v[vx] & 1 ? 1 : 0;
	reg.v[vx] >>= 1;
	reg.v[0x0f] = vf;
	reg.pc += 2;
}


// Subtracts register Vx from register Vy leaving the result in Vx. TODO: explain how this affects the flags.
void Chip8VM::i_subn_vx_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vy = memory[reg.pc + 1] >> 4;
	Byte vf = reg.v[vy] > reg.v[vx] ? 1 : 0;
	uint16_t result = reg.v[vy] - reg.v[vx];
	reg.v[vx] = result & 0xff;
	reg.v[0x0f] = vf;
	reg.pc += 2;
}


// Shifts register Vx left. TODO: explain how this affects the flags.
void Chip8VM::i_ld_vx_shl_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vf = reg.v[vx] & 0x80 ? 1 : 0;
	reg.v[vx] <<= 1;
	reg.v[0x0f] = vf;
	reg.pc += 2;
}


// Skips the next instruction if register Vx does not equal register Vy.
void Chip8VM::i_sne_vx_vy()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vy = memory[reg.pc + 1] >> 4;
	reg.pc += (reg.v[vx] != reg.v[vy]) ? 4 : 2;
}


// Sets the address register to an immediate value.
void Chip8VM::i_ld_i_addr()
{
	Address target = ((memory[reg.pc] & 0x0f) << 8) | memory[reg.pc + 1];
	reg.i = target;
	reg.pc += 2;
}


// Jumps to an address plus the contents of register V0.
void Chip8VM::i_jp_v0()
{
	Address target = ((memory[reg.pc] & 0x0f) << 8) | memory[reg.pc + 1];
	reg.pc = target + reg.v[0];
}


// Loads Vx with a random number, ANDed with an immediate value.
void Chip8VM::i_rnd_vx_imm()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte byte = memory[reg.pc + 1];
	reg.v[vx] = rnd() & byte;
	reg.pc += 2;
}


// Draws an N row sprite at the screen coordinates in registers Vx and Vy.
void Chip8VM::i_drw_vx_vy_n()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte vy = memory[reg.pc + 1] >> 4;
	Byte n = memory[reg.pc + 1] & 0x0f;
	auto x = reg.v[vx];
	auto y = reg.v[vy];
	bool vf = false;
	for (auto row = 0; row < n; row++)
	{
		auto data = memory[reg.i + row];
		auto sy = (y + row) & 0x1f;
		for (auto col = 0; col < 8; col++)
		{
			if (data & (0x80 >> col))
			{
				auto sx = (x + col) & 0x3f;
				vf = vf || io.screen.test(sx + sy * 64);
				io.screen.flip(sx + sy * 64);
			}
		}
	}
	reg.v[0x0f] = vf ? 1 : 0;

	reg.pc += 2;
}


// Skips the next instruction if the key whose value is in Vx is currently pressed.
void Chip8VM::i_skp_vx()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte key = reg.v[vx];
	reg.pc += (io.keys[key]) ? 4 : 2;
}


// Skips the next instruction if the key whose value is in Vx is not currently pressed.
void Chip8VM::i_sknp_vx()
{
	Byte vx = memory[reg.pc] & 0x0f;
	Byte key = reg.v[vx];
	reg.pc += (io.keys[key]) ? 2 : 4;
}


// Loads register Vx with the delay timer.
void Chip8VM::i_ld_vx_dt()
{
	Byte vx = memory[reg.pc] & 0x0f;
	reg.v[vx] = reg.dt;
	reg.pc += 2;
}


// Blocks the VM until a key is pressed and stores its value in register Vx.
void Chip8VM::i_ld_vx_k()
{
	is_blocked = (key == Key::NO_KEY);
	if (!is_blocked)
	{
		Byte vx = memory[reg.pc] & 0x0f;
		reg.v[vx] = static_cast<Byte>(key);
		key = Key::NO_KEY;
		reg.pc += 2;
	}
}


// Loads the delay timer with register Vx.
void Chip8VM::i_ld_dt_vx()
{
	Byte vx = memory[reg.pc] & 0x0f;
	reg.dt = reg.v[vx];
	reg.pc += 2;
}


// Loads the sound timer with register Vx.
void Chip8VM::i_ld_st_vx()
{
	Byte vx = memory[reg.pc] & 0x0f;
	reg.st = reg.v[vx];
	reg.pc += 2;
}


// Adds register Vx to the address register.
void Chip8VM::i_add_i_vx()
{
	Byte vx = memory[reg.pc] & 0x0f;
	reg.i += reg.v[vx];
	reg.pc += 2;
}


// Sets the address register to the location in VM memory of the font for the character in register Vx.
void Chip8VM::i_ld_f_vx()
{
	Byte vx = memory[reg.pc] & 0x0f;
	reg.i = reg.v[vx] * 5;
	reg.pc += 2;
}


// Stores the BCD representation of register Vx into VM memory starting at the address register.
void Chip8VM::i_ld_b_vx()
{
	Byte vx = memory[reg.pc] & 0x0f;
	auto address = reg.i;
	unsigned b = reg.v[vx];
	memory[address] = (b / 100) % 10;
	memory[address + 1] = (b / 10) % 10;
	memory[address + 2] = b % 10;
	reg.pc += 2;
}


// Stores registers V0..Vx into VM memory starting at the address register.
void Chip8VM::i_ld_i_vx()
{
	Byte vx = memory[reg.pc] & 0x0f;
	auto address = reg.i;
	for (auto i = 0; i <= vx; i++)
	{
		memory[address + i] = reg.v[i];
	}
	reg.pc += 2;
}


// Loads registers V0..Vx from VM memory starting at the address register.
void Chip8VM::i_ld_vx_i()
{
	Byte vx = memory[reg.pc] & 0x0f;
	auto address = reg.i;
	for (auto i = 0; i <= vx; i++)
	{
		reg.v[i] = memory[address + i];
	}
	reg.pc += 2;
}


// Decrements the delay timer. Should be called 60 times/second.
void Chip8VM::tick()
{
	if (reg.dt > 0)
	{
		--reg.dt;
	}
}


// Executes n instructions while the VM is not blocked.
void Chip8VM::step(uint32_t n)
{
	while (!is_blocked && n--)
	{
		Instruction op = shadow[reg.pc];
		(this->*op)();
	}
}


// Tells the VM that a key has just been pressed. Unblocks the VM if the VM is blocked.
void Chip8VM::key_pressed(Key key)
{
	if (key != Key::NO_KEY)
	{
		io.keys[static_cast<unsigned>(key)] = true;
		this->key = key;
		is_blocked = false;
	}
}


// Tells the VM that a key has just been released.
void Chip8VM::key_released(Key key)
{
	if (key != Key::NO_KEY)
	{
		io.keys[static_cast<unsigned>(key)] = false;
	}
}
