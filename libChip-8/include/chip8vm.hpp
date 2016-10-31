#pragma once

#include <array>
#include <bitset>
#include <random>
#include <string>


using namespace std;


class Chip8VM
{
public:
	static const int SCREEN_WIDTH = 64;
	static const int SCREEN_HEIGHT = 32;
	static const int MEMORY_SIZE = 4096;

	using Byte = uint8_t;
	using Address = uint16_t;
	using Opcode = uint16_t;

	// The Chip-8 VM's memory.
	array<Byte, MEMORY_SIZE> memory;

	struct {
		bitset<SCREEN_WIDTH * SCREEN_HEIGHT> screen;	// The screen memory.
		array<bool, 16> keys;							// The keyboard.
	} io;

	// Registers.
	struct {
		Address pc;					// The program counter.
		array<Byte, 16> v;			// General purpose registers (except for the flags register, VF).
		Address i;					// 16 bit address register.
		array<Address, 16> stack;	// The stack. Outside of VM memory.
		Byte sp;					// The stack pointer.
		Byte dt;					// Delay timer.
		Byte st;					// Sound timer.
	} reg;

	// Keycodes.
	enum class Key {
		NO_KEY = -1,
		KEY_0, KEY_1, KEY_2, KEY_3,
		KEY_4, KEY_5, KEY_6, KEY_7,
		KEY_8, KEY_9, KEY_A, KEY_B,
		KEY_C, KEY_D, KEY_E, KEY_F
	};

	// The most recent key that was pressed.
	Key key;

private:
	// An instruction is just a member function pointer.
	using Instruction = void(Chip8VM::*)();

	// The shadow memory contains compiled equivalents of the opcodes in VM memory.
	array<Instruction, MEMORY_SIZE> shadow;

	Address here;					// Purely used for 'compilation'.
	bool is_blocked;				// true if the emulator is blocked (on I/O)
	mt19937 random_number_engine;	// Mersenne Twister, for generating random numbers.

	// CHIP8 instructions. Mnemonics from http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.1.
	// Presented in numerical order.
	void i_illegal();			// xxxx - illegal instruction
	void i_cls();				// 00E0 - CLS
	void i_ret();				// 00EE - RET
	void i_jp();				// 1nnn - JP addr
	void i_call();				// 2nnn - CALL addr
	void i_se_vx_imm();			// 3xkk - SE Vx, byte
	void i_sne_vx_imm();		// 4xkk - SNE Vx, byte
	void i_se_vx_vy();			// 5xy0 - SE Vx, Vy
	void i_ld_vx_imm();			// 6xkk - LD Vx, byte
	void i_add_vx_imm();		// 7xkk - ADD Vx, byte
	void i_ld_vx_vy();			// 8xy0 - LD Vx, Vy
	void i_or_vx_vy();			// 8xy1 - OR Vx, Vy
	void i_and_vx_vy();			// 8xy2 - AND Vx, Vy
	void i_xor_vx_vy();			// 8xy3 - XOR Vx, Vy
	void i_add_vx_vy();			// 8xy4 - ADD Vx, Vy
	void i_sub_vx_vy();			// 8xy5 - SUB Vx, Vy
	void i_ld_vx_shr_vy();		// 8xy6 - SHR Vx {, Vy}
	void i_subn_vx_vy();		// 8xy7 - SUBN Vx, Vy
	void i_ld_vx_shl_vy();		// 8xyE - SHL Vx {, Vy}
	void i_sne_vx_vy();			// 9xy0 - SNE Vx, Vy
	void i_ld_i_addr();			// Annn - LD I, addr
	void i_jp_v0();				// Bnnn - JP V0, addr
	void i_rnd_vx_imm();		// Cxkk - RND Vx, byte
	void i_drw_vx_vy_n();		// Dxyn - DRW Vx, Vy, nibble
	void i_skp_vx();			// Ex9E - SKP Vx
	void i_sknp_vx();			// ExA1 - SKNP Vx
	void i_ld_vx_dt();			// Fx07 - LD Vx, DT
	void i_ld_vx_k();			// Fx0A - LD Vx, K
	void i_ld_dt_vx();			// Fx15 - LD DT, Vx
	void i_ld_st_vx();			// Fx18 - LD ST, Vx
	void i_add_i_vx();			// Fx1E - ADD I, Vx
	void i_ld_f_vx();			// Fx29 - LD F, Vx
	void i_ld_b_vx();			// Fx33 - LD B, Vx
	void i_ld_i_vx();			// Fx55 - LD [I], Vx
	void i_ld_vx_i();			// Fx65 - LD Vx, [I]

	Byte rnd();
	void push(Address address);
	Address pop();
	void write_ram(Opcode opcode);
	void write_shadow(Instruction i);
	Instruction instruction_from_opcode(Opcode opcode);

public:
	Chip8VM();

	void reset();
	void load(Byte* data, size_t len);
	void compile(Opcode opcode);
	void tick();
	void step(uint32_t n = 1);
	void key_pressed(Key key);
	void key_released(Key key);
};
