#include "globals.h"
#include "z80.h"
#ifndef ARDUINO
#include "abstraction_posix.h"
#endif
#include "console.h"

#define RAM_MM(self, a)   _Z80_getByte(self, a--)
#define RAM_PP(self, a)   _Z80_getByte(self, a++)

#define LOW_REGISTER(x)         ((x) & 0xff)
#define HIGH_REGISTER(x)        (((x) >> 8) & 0xff)

#define SET_LOW_REGISTER(x, v)  x = (((x) & 0xff00) | ((v) & 0xff))
#define SET_HIGH_REGISTER(x, v) x = (((x) & 0xff) | (((v) & 0xff) << 8))

#define ADDRMASK        0xffff

/* incTable[i] = (i & 0xa8) | (((i & 0xff) == 0) << 6) | (((i & 0xf) == 0) << 4), i = 0..256 */
static const uint8_t incTable[257] = {
	80,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,
	16,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,
	48, 32, 32, 32, 32, 32, 32, 32, 40, 40, 40, 40, 40, 40, 40, 40,
	48, 32, 32, 32, 32, 32, 32, 32, 40, 40, 40, 40, 40, 40, 40, 40,
	16,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,
	16,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,
	48, 32, 32, 32, 32, 32, 32, 32, 40, 40, 40, 40, 40, 40, 40, 40,
	48, 32, 32, 32, 32, 32, 32, 32, 40, 40, 40, 40, 40, 40, 40, 40,
	144,128,128,128,128,128,128,128,136,136,136,136,136,136,136,136,
	144,128,128,128,128,128,128,128,136,136,136,136,136,136,136,136,
	176,160,160,160,160,160,160,160,168,168,168,168,168,168,168,168,
	176,160,160,160,160,160,160,160,168,168,168,168,168,168,168,168,
	144,128,128,128,128,128,128,128,136,136,136,136,136,136,136,136,
	144,128,128,128,128,128,128,128,136,136,136,136,136,136,136,136,
	176,160,160,160,160,160,160,160,168,168,168,168,168,168,168,168,
	176,160,160,160,160,160,160,160,168,168,168,168,168,168,168,168, 80
};

/* decTable[i] = (i & 0xa8) | (((i & 0xff) == 0) << 6) | (((i & 0xf) == 0xf) << 4) | 2, i = 0..255 */
static const uint8_t decTable[256] = {
	66,  2,  2,  2,  2,  2,  2,  2, 10, 10, 10, 10, 10, 10, 10, 26,
	2,  2,  2,  2,  2,  2,  2,  2, 10, 10, 10, 10, 10, 10, 10, 26,
	34, 34, 34, 34, 34, 34, 34, 34, 42, 42, 42, 42, 42, 42, 42, 58,
	34, 34, 34, 34, 34, 34, 34, 34, 42, 42, 42, 42, 42, 42, 42, 58,
	2,  2,  2,  2,  2,  2,  2,  2, 10, 10, 10, 10, 10, 10, 10, 26,
	2,  2,  2,  2,  2,  2,  2,  2, 10, 10, 10, 10, 10, 10, 10, 26,
	34, 34, 34, 34, 34, 34, 34, 34, 42, 42, 42, 42, 42, 42, 42, 58,
	34, 34, 34, 34, 34, 34, 34, 34, 42, 42, 42, 42, 42, 42, 42, 58,
	130,130,130,130,130,130,130,130,138,138,138,138,138,138,138,154,
	130,130,130,130,130,130,130,130,138,138,138,138,138,138,138,154,
	162,162,162,162,162,162,162,162,170,170,170,170,170,170,170,186,
	162,162,162,162,162,162,162,162,170,170,170,170,170,170,170,186,
	130,130,130,130,130,130,130,130,138,138,138,138,138,138,138,154,
	130,130,130,130,130,130,130,130,138,138,138,138,138,138,138,154,
	162,162,162,162,162,162,162,162,170,170,170,170,170,170,170,186,
	162,162,162,162,162,162,162,162,170,170,170,170,170,170,170,186,
};

/* cbitsTable[i] = (i & 0x10) | ((i >> 8) & 1), i = 0..511 */
static const uint8_t cbitsTable[512] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
};

static uint8_t _Z80_getByte(Z80_instance_t* self, uint16_t addr)
{
	return self->ram[addr];
}

static void _Z80_putByte(Z80_instance_t* self, uint16_t addr, uint8_t value)
{
	self->ram[addr] = value;
}

static uint16_t _Z80_getWord(Z80_instance_t* self, uint16_t addr)
{
	return (uint16_t)(_Z80_getByte(self, addr)) |
	    (uint16_t)(_Z80_getByte(self, addr + 1) << 8);
}

static void _Z80_putWord(Z80_instance_t* self, uint32_t Addr, register uint32_t Value)
{
	_Z80_putByte(self, Addr, Value & 0xFF);
	_Z80_putByte(self, ++Addr, Value >> 8);
}

void
Z80_reset(Z80_instance_t* self)
{
	self->PC = 0;
	self->IFF = 0;
	self->IR = 0;
	self->Status = 0;
	self->Debug = 0;
	self->Break = -1;
	self->Step = -1;
}

void Z80_step(Z80_instance_t* self)
{
#define AF (self->AF)
#define AF1 (self->AF1)
#define BC (self->BC)
#define DE (self->DE)
#define PC (self->PC)
#define HL (self->HL)
#define PCX (self->PCX)
#define SP (self->SP)
	
	uint32_t temp = 0;
#define SET_PV2(x)  ((temp == (x)) << 2)
	
	PCX = PC;

	switch (RAM_PP(self, PC)) {
		
	case 0x00: /* NOP */
		break;

	case 0x01: /* LD BC,nnnn */
		BC = _Z80_getWord(self, PC);
		PC += 2;
		break;

	case 0x02: /* LD (BC),A */
		_Z80_putByte(self, BC, HIGH_REGISTER(AF));
		break;
		
	case 0x03:      /* INC BC */
		++BC;
		break;
	
	case 0x04:      /* INC B */
		BC += 0x100;
		temp = HIGH_REGISTER(BC);
		AF = (AF & ~0xfe) | incTable[temp] | SET_PV2(0x80); /* SET_PV2 uses temp */
		break;
		
	case 0x05:      /* DEC B */
		BC -= 0x100;
		temp = HIGH_REGISTER(BC);
		AF = (AF & ~0xfe) | decTable[temp] | SET_PV2(0x7f); /* SET_PV2 uses temp */
		break;

	case 0x06:      /* LD B,nn */
		SET_HIGH_REGISTER(BC, RAM_PP(self, PC));
		break;
		
	case 0x07:      /* RLCA */
		AF = ((AF >> 7) & 0x0128) | ((AF << 1) & ~0x1ff) |
			(AF & 0xc4) | ((AF >> 15) & 1);
		break;
		
	case 0x08:      /* EX AF,AF' */
		temp = AF;
		AF = AF1;
		AF1 = temp;
		break;
		
	case 0x09:      /* ADD HL,BC */
		HL &= ADDRMASK;
		BC &= ADDRMASK;
		temp = HL + BC;
		AF = (AF & ~0x3b) | ((temp >> 8) & 0x28) | cbitsTable[(HL ^ BC ^ temp) >> 8];
		HL = temp;
		break;
		
	case 0x0a:      /* LD A,(BC) */
		SET_HIGH_REGISTER(AF, _Z80_getByte(self, BC));
		break;
		
	case 0x0b:      /* DEC BC */
		--BC;
		break;
		
	case 0x0c:      /* INC C */
		temp = LOW_REGISTER(BC) + 1;
		SET_LOW_REGISTER(BC, temp);
		AF = (AF & ~0xfe) | incTable[temp] | SET_PV2(0x80);
		break;
	
	case 0x0d:      /* DEC C */
		temp = LOW_REGISTER(BC) - 1;
		SET_LOW_REGISTER(BC, temp);
		AF = (AF & ~0xfe) | decTable[temp & 0xff] | SET_PV2(0x7f);
		break;
		
	case 0x13:      /* INC DE */
		++DE;
		break;
		
	case 0x1b:      /* DEC DE */
		--DE;
		break;
		
	case 0x23:      /* INC HL */
		++HL;
		break;
		
	case 0x2b:      /* DEC HL */
		--HL;
		break;
		
	case 0x33:      /* INC SP */
		++SP;
		break;
		
	case 0x3b:      /* DEC SP */
		--SP;
		break;
		
	case 0x76:      /* HALT */
#ifdef DEBUG
		_puts("\r\n::CPU HALTED::");	// A halt is a good indicator of broken code
		_puts("Press any key...");
		_getch();
#endif
		--PC;
		break;
	}
	
#undef AF
#undef AF1
#undef BC
#undef DE
#undef HL
#undef PC
#undef PCX
#undef SP
}
