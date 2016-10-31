#include "catch.hpp"

#include <libChip-8\include\chip8vm.hpp>


TEST_CASE("Startup")
{
	Chip8VM vm;

	SECTION("PC is set to 0x200")
	{
		REQUIRE(vm.reg.pc == 0x200);
	}
}


TEST_CASE("Instructions")
{
	Chip8VM vm;

	SECTION("CLS")
	{
		vm.io.screen.reset();
		vm.compile(0x00e0);
		vm.step();
		REQUIRE(vm.io.screen.none());
		REQUIRE(vm.reg.pc == 0x202);
	}

	SECTION("JP addr")
	{
		vm.compile(0x1350); 	// JP 350H
		vm.step();
		REQUIRE(vm.reg.pc == 0x350);
	}

	SECTION("CALL and RET")
	{
		vm.compile(0x2204); 	// CALL 204H
		vm.compile(0x1350);		// JP 350H (ignored)
		vm.compile(0x00ee);		// RET
		vm.step(2);
		REQUIRE(vm.reg.pc == 0x202);
	}

	SECTION("SE Vx, imm")
	{
		SECTION("equal")
		{
			vm.compile(0x6299);		// LD V2, 99H
			vm.compile(0x3299);		// SE V2, 99H
			vm.compile(0x1350);		// JP 350H (skipped)
			vm.step(2);
			REQUIRE(vm.reg.pc == 0x206);
		}

		SECTION("not equal")
		{
			vm.compile(0x6291);		// LD V2, 91H
			vm.compile(0x3299);		// SE V2, 99H
			vm.compile(0x1350);		// JP 350H
			vm.step(3);
			REQUIRE(vm.reg.pc == 0x350);
		}
	}

	SECTION("SNE Vx, imm")
	{
		SECTION("equal")
		{
			vm.compile(0x6299);		// LD V2, 99H
			vm.compile(0x4299);		// SNE V2, 99H
			vm.compile(0x1350);		// JP 350H
			vm.step(3);
			REQUIRE(vm.reg.pc == 0x350);
		}

		SECTION("not equal")
		{
			vm.compile(0x6291);		// LD V2, 91H
			vm.compile(0x4299);		// SNE V2, 99H
			vm.compile(0x1350);		// JP 350H (skipped)
			vm.step(2);
			REQUIRE(vm.reg.pc == 0x206);
		}
	}

	SECTION("SE Vx, Vy")
	{
		SECTION("equal")
		{
			vm.compile(0x6299);		// LD V2, 99H
			vm.compile(0x6499);		// LD V4, 99H
			vm.compile(0x5240);		// SE V2, V4
			vm.compile(0x1350);		// JP 350H (skipped)
			vm.step(3);
			REQUIRE(vm.reg.pc == 0x208);
		}

		SECTION("not equal")
		{
			vm.compile(0x6299);		// LD V2, 99H
			vm.compile(0x6439);		// LD V4, 39H
			vm.compile(0x5240);		// SE V2, V4
			vm.compile(0x1350);		// JP 350H
			vm.step(4);
			REQUIRE(vm.reg.pc == 0x350);
		}
	}

	SECTION("LD Vx, byte")
	{
		vm.reg.v[1] = 0;
		vm.compile(0x6188); 	// LD V1, 88H
		vm.step();
		REQUIRE(vm.reg.v[1] == 0x88);
		REQUIRE(vm.reg.pc == 0x202);
	}

	SECTION("ADD Vx, byte")
	{
		vm.compile(0x6502);		// LD V5, 2H
		vm.compile(0x7510);		// ADD V5, 10H
		vm.step(2);
		REQUIRE(vm.reg.v[5] == 0x12);
		REQUIRE(vm.reg.pc == 0x204);
	}

	SECTION("LD Vx, Vy")
	{
		vm.compile(0x6099);		// LD V0, 99H
		vm.compile(0x8200);		// LD V2, V0
		vm.step(2);
		REQUIRE(vm.reg.v[2] == 0x99);
		REQUIRE(vm.reg.pc == 0x204);
	}

	SECTION("OR Vx, Vy")
	{
		vm.compile(0x6011);		// LD V0, 11H
		vm.compile(0x6122);		// LD V1, 22H
		vm.compile(0x8101);		// OR V1, V0
		vm.step(3);
		REQUIRE(vm.reg.v[1] == 0x33);
		REQUIRE(vm.reg.pc == 0x206);
	}

	SECTION("AND Vx, Vy")
	{
		vm.compile(0x60F7);		// LD V0, F7H
		vm.compile(0x697E);		// LD V9, 77H
		vm.compile(0x8902);		// AND V9, V0
		vm.step(3);
		REQUIRE(vm.reg.v[9] == 0x76);
		REQUIRE(vm.reg.pc == 0x206);
	}

	SECTION("XOR Vx, Vy")
	{
		vm.compile(0x601F);		// LD V0, 1FH
		vm.compile(0x6AF1);		// LD VA, F1H
		vm.compile(0x8A03);		// XOR VA, V0
		vm.step(3);
		REQUIRE(vm.reg.v[0x0a] == 0xee);
		REQUIRE(vm.reg.pc == 0x206);
	}

	SECTION("ADD Vx, Vy")
	{
		SECTION("no carry")
		{
			vm.compile(0x6233);		// LD V2, 33H
			vm.compile(0x6344);		// LD V3, 44H
			vm.compile(0x8234);		// ADD V2, V3
			vm.step(3);
			REQUIRE(vm.reg.v[0x02] == 0x77);
			REQUIRE(vm.reg.v[0x0f] == 0x00);
			REQUIRE(vm.reg.pc == 0x206);
		}

		SECTION("carry")
		{
			vm.compile(0x6280);		// LD V2, 80H
			vm.compile(0x6380);		// LD V3, 80H
			vm.compile(0x8234);		// ADD V2, V3
			vm.step(3);
			REQUIRE(vm.reg.v[0x02] == 0x00);
			REQUIRE(vm.reg.v[0x0f] != 0x00);
			REQUIRE(vm.reg.pc == 0x206);
		}
	}

	SECTION("SUB Vx, Vy")
	{
		SECTION("no borrow")
		{
			vm.compile(0x6677);		// LD V6, 77H
			vm.compile(0x6502);		// LD V5, 02H
			vm.compile(0x8655);		// SUB V6, V5
			vm.step(3);
			REQUIRE(vm.reg.v[0x06] == 0x75);
			REQUIRE(vm.reg.v[0x0f] != 0x00);
			REQUIRE(vm.reg.pc == 0x206);
		}

		SECTION("borrow")
		{
			vm.compile(0x6602);		// LD V6, 02H
			vm.compile(0x6503);		// LD V5, 03H
			vm.compile(0x8655);		// SUB V6, V5
			vm.step(3);
			REQUIRE(vm.reg.v[0x06] == 0xFF);
			REQUIRE(vm.reg.v[0x0f] == 0x00);
			REQUIRE(vm.reg.pc == 0x206);
		}
	}

	SECTION("SHR Vx")
	{
		SECTION("carry")
		{
			vm.compile(0x6913);		// LD V9, 13H
			vm.compile(0x8906);		// SHR V9
			vm.step(2);
			REQUIRE(vm.reg.v[0x09] == 0x09);
			REQUIRE(vm.reg.v[0x0f] != 0x00);
		}

		SECTION("no carry")
		{
			vm.compile(0x6912);		// LD V9, 12H
			vm.compile(0x8906);		// SHR V9
			vm.step(2);
			REQUIRE(vm.reg.v[0x09] == 0x09);
			REQUIRE(vm.reg.v[0x0f] == 0x00);
		}
	}

	SECTION("SUBN Vx, Vy")
	{
		SECTION("no borrow")
		{
			vm.compile(0x6602);		// LD V6, 02H
			vm.compile(0x6577);		// LD V5, 77H
			vm.compile(0x8657);		// SUBN V6, V5
			vm.step(3);
			REQUIRE(vm.reg.v[0x06] == 0x75);
			REQUIRE(vm.reg.v[0x0f] != 0x00);
		}

		SECTION("borrow")
		{
			vm.compile(0x6603);		// LD V6, 03H
			vm.compile(0x6502);		// LD V5, 02H
			vm.compile(0x8657);		// SUBN V6, V5
			vm.step(3);
			REQUIRE(vm.reg.v[0x06] == 0xFF);
			REQUIRE(vm.reg.v[0x0f] == 0x00);
			REQUIRE(vm.reg.pc == 0x206);
		}
	}

	SECTION("SHL Vx")
	{
		SECTION("carry")
		{
			vm.compile(0x6881);		// LD V8, 81H
			vm.compile(0x880E);		// SHL V8
			vm.step(2);
			REQUIRE(vm.reg.v[0x08] == 0x02);
			REQUIRE(vm.reg.v[0x0f] != 0x00);
		}

		SECTION("no carry")
		{
			vm.compile(0x6809);		// LD V8, 09H
			vm.compile(0x880E);		// SHL V8
			vm.step(2);
			REQUIRE(vm.reg.v[0x08] == 0x12);
			REQUIRE(vm.reg.v[0x0f] == 0x00);
		}
	}

	SECTION("SNE Vx, Vy")
	{
		SECTION("equal")
		{
			vm.compile(0x6299);		// LD V2, 99H
			vm.compile(0x6499);		// LD V4, 99H
			vm.compile(0x9240);		// SNE V2, V4
			vm.compile(0x1350);		// JP 350H
			vm.step(4);
			REQUIRE(vm.reg.pc == 0x350);
		}

		SECTION("not equal")
		{
			vm.compile(0x6299);		// LD V2, 99H
			vm.compile(0x6439);		// LD V4, 39H
			vm.compile(0x9240);		// SNE V2, V4
			vm.compile(0x1350);		// JP 350H (skipped)
			vm.step(3);
			REQUIRE(vm.reg.pc == 0x208);
		}
	}

	SECTION("LD I, addr")
	{
		vm.compile(0xA550);			// LD I, 550H
		vm.step();
		REQUIRE(vm.reg.i == 0x550);
		REQUIRE(vm.reg.pc == 0x202);
	}

	SECTION("JP V0, addr")
	{
		vm.compile(0x6040);			// LD V0, 40H
		vm.compile(0xB122);			// JP V0, 122H
		vm.step(2);
		REQUIRE(vm.reg.pc == 0x162);
	}

	SECTION("RND Vx, byte")
	{
		vm.compile(0xc155);			// RND V1, 55H
		vm.compile(0xc2aa);			// RND V2, AAH
		vm.compile(0xc3f0);			// RND V3, F0H
		vm.compile(0xc40f);			// RND V4, 0FH
		vm.step(4);
		REQUIRE((vm.reg.v[0x01] & 0xaa) == 0);
		REQUIRE((vm.reg.v[0x02] & 0x55) == 0);
		REQUIRE((vm.reg.v[0x03] & 0x0f) == 0);
		REQUIRE((vm.reg.v[0x04] & 0xf0) == 0);
		REQUIRE(vm.reg.pc == 0x208);
	}

	SECTION("DRW Vx, Vy, nibble")
	{
		// XOR a pattern onto the screen and check for its presence.
		vm.reg.v[0x0] = 4;
		vm.reg.v[0x1] = 5;
		vm.reg.i = 0x100;
		vm.memory[0x100] = 0xff;
		vm.memory[0x101] = 0x81;
		vm.memory[0x102] = 0x81;
		vm.memory[0x103] = 0xff;
		vm.compile(0xD018);			// DRW V0, V1, 8
		vm.step(1);
		for (auto y = 5; y < 9; y++)
		{
			for (auto x = 4; x < 12; x++)
			{
				if (y == 5 || y == 8 || x == 4 || x == 11)
				{
					REQUIRE(vm.io.screen.test(x + 64 * y));
				}
			}
		}
		REQUIRE(vm.reg.v[0x0f] == 0);

		// XOR it off again. This time the collision flag should be set.
		vm.compile(0xD018);
		vm.step(1);
		for (auto y = 5; y < 9; y++)
		{
			for (auto x = 4; x < 12; x++)
			{
				if (y == 5 || y == 8 || x == 4 || x == 11)
				{
					REQUIRE(!vm.io.screen.test(x + 64 * y));
				}
			}
		}
		REQUIRE(vm.reg.v[0x0f] != 0);

		// Draw some fonts. Not really unit tests, but you get to see the screen.
		for (auto i = 0; i < 16; i++)
		{
			vm.reg.v[0x0] = 1 + (i & 7) * 6;
			vm.reg.v[0x1] = 8 + (i & 8);
			vm.reg.v[0x07] = i;
			vm.compile(0xf729);			// LD F, V7
			vm.compile(0xD015);			// DRW V0, V1, 5
			vm.step(2);
		}
	}

	SECTION("SKP Vx")
	{
		SECTION("key pressed")
		{
			vm.key_pressed(Chip8VM::Key::KEY_C);
			vm.compile(0x610c);			// LD V1, 0CH
			vm.compile(0xe19e);			// SKP V1
			vm.compile(0x1350);			// JP 350H (skipped)
			vm.step(2);
			REQUIRE(vm.reg.pc == 0x206);
		}

		SECTION("key not pressed")
		{
			vm.key_released(Chip8VM::Key::KEY_C);
			vm.compile(0x610c);			// LD V1, 0CH
			vm.compile(0xe19e);			// SKP V1
			vm.compile(0x1350);			// JP 350H
			vm.step(3);
			REQUIRE(vm.reg.pc == 0x350);
		}
	}

	SECTION("SKNP Vx")
	{
		SECTION("key pressed")
		{
			vm.key_pressed(Chip8VM::Key::KEY_C);
			vm.compile(0x610c);			// LD V1, 0CH
			vm.compile(0xe1a1);			// SKNP V1
			vm.compile(0x1350);			// JP 350H
			vm.step(3);
			REQUIRE(vm.reg.pc == 0x350);
		}

		SECTION("key not pressed")
		{
			vm.key_released(Chip8VM::Key::KEY_C);
			vm.compile(0x610c);			// LD V1, 0CH
			vm.compile(0xe1a1);			// SKNP V1
			vm.compile(0x1350);			// JP 350H (skipped)
			vm.step(2);
			REQUIRE(vm.reg.pc == 0x206);
		}
	}

	SECTION("LD Vx, DT")
	{
		vm.reg.dt = 0x39;
		vm.compile(0xfb07);			// LD VB, DT
		vm.step();
		REQUIRE(vm.reg.v[0x0b] == vm.reg.dt);
		REQUIRE(vm.reg.pc == 0x202);
	}

	SECTION("LD Vx, K")
	{
		SECTION("key not pressed")
		{
			vm.reg.v[0x03] = 0x55;		// A value that has nothing to do with the keyboard.
			vm.compile(0xf30a);			// LD V3, K
			vm.step(10);				// It blocks, because no key is ever pressed.
			REQUIRE(vm.reg.v[0x03] == 0x55);
			REQUIRE(vm.reg.pc == 0x200);
		}

		SECTION("key pressed")
		{
			vm.reg.v[0x03] = 0x55;		// A value that has nothing to do with the keyboard.
			vm.compile(0xf30a);			// LD V3, K
			vm.step();
			vm.key_pressed(Chip8VM::Key::KEY_1);
			vm.step();
			REQUIRE(vm.reg.v[0x03] == static_cast<Chip8VM::Byte>(Chip8VM::Key::KEY_1));
			REQUIRE(vm.reg.pc == 0x202);
		}

		SECTION("key released")
		{
			vm.reg.v[0x03] = 0x55;		// A value that has nothing to do with the keyboard.
			vm.compile(0xf30a);			// LD V3, K
			vm.step();
			vm.key_released(Chip8VM::Key::KEY_1);
			vm.step();
			REQUIRE(vm.reg.v[0x03] == 0x55);
			REQUIRE(vm.reg.pc == 0x200);
		}
	}

	SECTION("LD DT, Vx")
	{
		vm.compile(0x6944);			// LD V9, 44H
		vm.compile(0xf915);			// LD DT, V9
		vm.step(2);
		REQUIRE(vm.reg.dt == vm.reg.v[0x09]);
		REQUIRE(vm.reg.pc == 0x204);
	}

	SECTION("LD ST, Vx")
	{
		vm.compile(0x6623);			// LD V6, 23H
		vm.compile(0xf618);			// LD ST, V6
		vm.step(2);
		REQUIRE(vm.reg.st == vm.reg.v[0x06]);
		REQUIRE(vm.reg.pc == 0x204);
	}

	SECTION("ADD I, Vx")
	{
		vm.reg.i = 0x500;
		vm.compile(0x6715);			// LD V7, 15H
		vm.compile(0xf71e);			// ADD I, V7
		vm.step(2);
		REQUIRE(vm.reg.i == 0x515);
		REQUIRE(vm.reg.pc == 0x204);
	}

	SECTION("LD F, Vx")
	{
		vm.reg.v[0x07] = 4;
		vm.compile(0xf729);			// LD F, V7
		vm.step();
		REQUIRE(vm.reg.i == vm.reg.v[0x07] * 5);
		REQUIRE(vm.reg.pc == 0x202);
	}

	SECTION("LD B, Vx")
	{
		vm.reg.v[0xe] = 123;
		vm.reg.i = 0x300;
		vm.compile(0xfe33);			// LD B, VE
		vm.step();
		REQUIRE(vm.memory[0x300] == vm.reg.v[0xe] / 100);

		REQUIRE(vm.memory[0x301] == (vm.reg.v[0xe] % 100) / 10);
		REQUIRE(vm.memory[0x302] == vm.reg.v[0xe] % 10);
		REQUIRE(vm.reg.pc == 0x202);
	}

	SECTION("LD [I], Vx")
	{
		for (auto i = 0; i <= 15; i++)
		{
			vm.reg.v[i] = i + 1;
		}
		vm.reg.i = 0x300;
		vm.compile(0xff55);			// LD [I], VF
		vm.step();
		for (auto i = 0; i <= 15; i++)
		{
			REQUIRE(vm.memory[0x300 + i] == i + 1);
		}
		REQUIRE(vm.reg.pc == 0x202);
	}

	SECTION("LD Vx, [I]")
	{
		for (auto i = 0; i <= 15; i++)
		{
			vm.memory[0x300 + i] = i + 1;
		}
		vm.reg.i = 0x300;
		vm.compile(0xff65);			// LD VF, [I]
		vm.step();
		for (auto i = 0; i <= 15; i++)
		{
			REQUIRE(vm.reg.v[i] == i + 1);
		}
		REQUIRE(vm.reg.pc == 0x202);
	}
}
