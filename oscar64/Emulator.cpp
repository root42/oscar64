#include "Emulator.h"
#include <stdio.h>

Emulator::Emulator(void)
{
	for (int i = 0; i < 0x10000; i++)
		mMemory[i] = 0;
}


Emulator::~Emulator(void)
{

}

static const uint8 STATUS_SIGN = 0x80;
static const uint8 STATUS_OVERFLOW = 0x40;
static const uint8 STATUS_ZERO = 0x02;
static const uint8 STATUS_CARRY = 0x01;

void Emulator::UpdateStatus(uint8 result)
{
	mRegP &= ~(STATUS_ZERO | STATUS_SIGN);
	if (result == 0) mRegP |= STATUS_ZERO;
	if (result & 0x80) mRegP |= STATUS_SIGN;
}

void Emulator::UpdateStatusCarry(uint8 result, bool carry) 
{
	mRegP &= ~(STATUS_ZERO | STATUS_SIGN | STATUS_CARRY);
	if (result == 0) mRegP |= STATUS_ZERO;
	if (result & 0x80) mRegP |= STATUS_SIGN;
	if (carry)
		mRegP |= STATUS_CARRY;
}

void Emulator::DumpCycles(void)
{
	int		numTops	= 0;
	int		topIP[101], topCycles[101];

	int		totalCycles = 0;

	for (int i = 0; i < 0x10000; i++)
	{
		int	cycles = mCycles[i];
		totalCycles += cycles;
		if (cycles > 0)
		{
			if (numTops == 0 || cycles > topCycles[numTops])
			{
				if (numTops < 40)
					numTops++;
				int j = numTops;
				while (j > 0 && topCycles[j-1] < cycles)
				{
					topCycles[j] = topCycles[j - 1];
					topIP[j] = topIP[j - 1];
					j--;
				}
				topCycles[j] = cycles;
				topIP[j] = i;
			}
		}
	}
	
	printf("Total Cycles %d\n", totalCycles);
	return;

	for (int i = 0; i < numTops; i++)
	{
		printf("  %2d : %04x : %d\n", i, topIP[i], topCycles[i]);
	}

}

bool Emulator::EmulateInstruction(AsmInsType type, AsmInsMode mode, int addr, int & cycles)
{
	int	t;

	switch (type)
	{
	case ASMIT_ADC:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		t = mRegA + addr + (mRegP & STATUS_CARRY);

		mRegP = 0;

		if ((mRegA & 0x80) && (addr & 0x80) && !(t & 0x80) ||
			!(mRegA & 0x80) && !(addr & 0x80) && (t & 0x80))
			mRegP |= STATUS_OVERFLOW;

		mRegA = (t & 255);
		UpdateStatusCarry(mRegA, t >= 256);
		break;
	case ASMIT_AND:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		mRegA &= addr;
		UpdateStatus(mRegA);
		break;
	case ASMIT_ASL:
		if (mode == ASMIM_IMPLIED)
		{
			t = mRegA << 1;
			mRegA = (t & 255);
			UpdateStatusCarry(mRegA, t >= 256);
		}
		else
		{
			t = mMemory[addr] << 1;
			mMemory[addr] = t & 255;
			UpdateStatusCarry(t & 255, t >= 256);
			cycles += 2;
		}
		break;
	case ASMIT_BCC:
		if (!(mRegP & STATUS_CARRY))
			mIP = addr;
		break;
	case ASMIT_BCS:
		if ((mRegP & STATUS_CARRY))
			mIP = addr;
		break;
	case ASMIT_BEQ:
		if ((mRegP & STATUS_ZERO))
			mIP = addr;
		break;
	case ASMIT_BIT:
		t = mMemory[addr];
		mRegP &= ~(STATUS_ZERO | STATUS_SIGN | STATUS_OVERFLOW);
		if (t & 0x80) mRegP |= STATUS_SIGN;
		if (t & 0x40) mRegP |= STATUS_OVERFLOW;
		if (!(t & mRegA)) mRegP |= STATUS_ZERO;
		break;
	case ASMIT_BMI:
		if ((mRegP & STATUS_SIGN))
		{
			mIP = addr;
			cycles++;
		}
		break;
	case ASMIT_BNE:
		if (!(mRegP & STATUS_ZERO))
		{
			mIP = addr;
			cycles++;
		}
		break;
	case ASMIT_BPL:
		if (!(mRegP & STATUS_SIGN))
		{
			mIP = addr;
			cycles++;
		}
		break;
	case ASMIT_BRK:
		return false;
		break;
	case ASMIT_BVC:
		if (!(mRegP & STATUS_OVERFLOW))
		{
			mIP = addr;
			cycles++;
		}
		break;
	case ASMIT_BVS:
		if ((mRegP & STATUS_OVERFLOW))
		{
			mIP = addr;
			cycles++;
		}
		break;
	case ASMIT_CLC:
		mRegP &= ~STATUS_CARRY;
		break;
	case ASMIT_CLD:
		break;
	case ASMIT_CLI:
		break;
	case ASMIT_CLV:
		mRegP &= ~STATUS_OVERFLOW;
		break;
	case ASMIT_CMP:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		t = mRegA + (addr ^ 0xff) + 1;

		mRegP = 0;

		if ((mRegA & 0x80) && !(addr & 0x80) && !(t & 0x80) ||
			!(mRegA & 0x80) && (addr & 0x80) && (t & 0x80))
			mRegP |= STATUS_OVERFLOW;

		UpdateStatusCarry(t & 255, t >= 256);
		break;
	case ASMIT_CPX:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		t = mRegX + (addr ^ 0xff) + 1;

		mRegP = 0;

		if ((mRegX & 0x80) && !(addr & 0x80) && !(t & 0x80) ||
			!(mRegX & 0x80) && (addr & 0x80) && (t & 0x80))
			mRegP |= STATUS_OVERFLOW;

		UpdateStatusCarry(t & 255, t >= 256);
		break;
	case ASMIT_CPY:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		t = mRegY + (addr ^ 0xff) + 1;

		mRegP = 0;

		if ((mRegY & 0x80) && !(addr & 0x80) && !(t & 0x80) ||
			!(mRegY & 0x80) && (addr & 0x80) && (t & 0x80))
			mRegP |= STATUS_OVERFLOW;

		UpdateStatusCarry(t & 255, t >= 256);
		break;
	case ASMIT_DEC:
		if (mode == ASMIM_IMPLIED)
		{
			t = mRegA - 1;
			mRegA = (t & 255);
			UpdateStatus(mRegA);
		}
		else
		{
			t = mMemory[addr] - 1;
			mMemory[addr] = t & 255;
			UpdateStatus(t & 255);
			cycles += 2;
		}
		break;
	case ASMIT_DEX:
		t = mRegX - 1;
		mRegX = (t & 255);
		UpdateStatus(mRegX);
		break;
	case ASMIT_DEY:
		t = mRegY - 1;
		mRegY = (t & 255);
		UpdateStatus(mRegY);
		break;
	case ASMIT_EOR:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		mRegA ^= addr;
		UpdateStatus(mRegA);
		break;
	case ASMIT_INC:
		if (mode == ASMIM_IMPLIED)
		{
			t = mRegA + 1;
			mRegA = (t & 255);
			UpdateStatus(mRegA);
		}
		else
		{
			t = mMemory[addr] + 1;
			mMemory[addr] = t & 255;
			UpdateStatus(t & 255);
			cycles += 2;
		}
		break;
	case ASMIT_INX:
		t = mRegX + 1;
		mRegX = (t & 255);
		UpdateStatus(mRegX);
		break;
	case ASMIT_INY:
		t = mRegY + 1;
		mRegY = (t & 255);
		UpdateStatus(mRegY);
		break;
	case ASMIT_JMP:
		mIP = addr;
		break;
	case ASMIT_JSR:
		mRegS--;
		mMemory[0x100 + mRegS] = (mIP - 1) >> 8;
		mRegS--;
		mMemory[0x100 + mRegS] = (mIP - 1) & 0xff;
		mIP = addr;
		cycles += 2;
		break;
	case ASMIT_LDA:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		mRegA = addr;
		UpdateStatus(mRegA);
		break;
	case ASMIT_LDX:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		mRegX = addr;
		UpdateStatus(mRegX);
		break;
	case ASMIT_LDY:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		mRegY = addr;
		UpdateStatus(mRegY);
		break;
	case ASMIT_LSR:
		if (mode == ASMIM_IMPLIED)
		{
			int	c = mRegA & 1;
			t = mRegA >> 1;
			mRegA = (t & 255);
			UpdateStatusCarry(mRegA, c != 0);
		}
		else
		{
			int	c = mMemory[addr] & 1;
			t = mMemory[addr] >> 1;
			mMemory[addr] = t & 255;
			UpdateStatusCarry(t & 255, c != 0);
			cycles += 2;
		}
		break;
	case ASMIT_NOP:
		break;
	case ASMIT_ORA:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		mRegA |= addr;
		UpdateStatus(mRegA);
		break;
	case ASMIT_PHA:
		mRegS--;
		mMemory[0x100 + mRegS] = mRegA;
		cycles ++;
		break;
	case ASMIT_PHP:
		mRegS--;
		mMemory[0x100 + mRegS] = mRegP;
		cycles++;
		break;
	case ASMIT_PLA:
		mRegA = mMemory[0x100 + mRegS];
		mRegS++;
		cycles++;
		break;
	case ASMIT_PLP:
		mRegP = mMemory[0x100 + mRegS];
		mRegS++;
		cycles++;
		break;
	case ASMIT_ROL:
		if (mode == ASMIM_IMPLIED)
		{
			t = (mRegA << 1) | (mRegP & STATUS_CARRY);
			mRegA = (t & 255);
			UpdateStatusCarry(mRegA, t >= 256);
		}
		else
		{
			t = (mMemory[addr] << 1) | (mRegP & STATUS_CARRY);;
			mMemory[addr] = t & 255;
			UpdateStatusCarry(t & 255, t >= 256);
			cycles+=2;
		}
		break;
	case ASMIT_ROR:
		if (mode == ASMIM_IMPLIED)
		{
			int	c = mRegA & 1;
			t = (mRegA >> 1) | ((mRegP & STATUS_CARRY) << 7);
			mRegA = (t & 255);
			UpdateStatusCarry(mRegA, c != 0);
		}
		else
		{
			int	c = mMemory[addr] & 1;
			t = (mMemory[addr] >> 1) | ((mRegP & STATUS_CARRY) << 7);
			mMemory[addr] = t & 255;
			UpdateStatusCarry(t & 255, c != 0);
			cycles += 2;
		}
		break;
	case ASMIT_RTI:
		break;
	case ASMIT_RTS:
		mIP = (mMemory[0x100 + mRegS] + 256 * mMemory[0x101 + mRegS] + 1) & 0xffff;
		mRegS += 2;
		cycles += 4;
		break;
	case ASMIT_SBC:
		if (mode != ASMIM_IMMEDIATE)
			addr = mMemory[addr];
		t = mRegA + (addr ^ 0xff) + (mRegP & STATUS_CARRY);

		mRegP = 0;

		if ((mRegA & 0x80) && !(addr & 0x80) && !(t & 0x80) ||
			!(mRegA & 0x80) && (addr & 0x80) && (t & 0x80))
			mRegP |= STATUS_OVERFLOW;

		mRegA = (t & 255);
		UpdateStatusCarry(t & 255, t >= 256);
		break;
	case ASMIT_SEC:
		mRegP |= STATUS_CARRY;
		break;
	case ASMIT_SED:
		break;
	case ASMIT_SEI:
		break;
	case ASMIT_STA:
		mMemory[addr] = mRegA;
		break;
	case ASMIT_STX:
		mMemory[addr] = mRegX;
		break;
	case ASMIT_STY:
		mMemory[addr] = mRegY;
		break;
	case ASMIT_TAX:
		mRegX = mRegA;
		UpdateStatus(mRegX);
		break;
	case ASMIT_TAY:
		mRegY = mRegA;
		UpdateStatus(mRegY);
		break;
	case ASMIT_TSX:
		mRegX = mRegS;
		UpdateStatus(mRegX);
		break;
	case ASMIT_TXA:
		mRegA = mRegX;
		UpdateStatus(mRegA);
		break;
	case ASMIT_TXS:
		mRegS = mRegX;
		break;
	case ASMIT_TYA:
		mRegA = mRegY;
		UpdateStatus(mRegA);
		break;
	case ASMIT_INV:
		return false;
		break;
	}

	return true;
}

int Emulator::Emulate(int startIP)
{
	int	trace = 0;

	for (int i = 0; i < 0x10000; i++)
		mCycles[i] = 0;

	mIP = startIP;
	mRegA = 0;
	mRegX = 0;
	mRegY = 0;
	mRegP = 0;
	mRegS = 0xfe;

	mMemory[0x1fe] = 0xff;
	mMemory[0x1ff] = 0xff;

	while (mIP != 0)
	{
		if (mIP == 0xffd2)
		{
			if (mRegA == 13)
				putchar('\n');
			else
				putchar(mRegA);
			mIP = mMemory[0x100 + mRegS] + 256 * mMemory[0x101 + mRegS] + 1;
			mRegS += 2;
		}

		uint8	opcode = mMemory[mIP];
		AsmInsData	d = DecInsData[opcode];
		int	addr = 0, taddr;
		int	ip = mIP;
		int	iip = mMemory[BC_REG_IP] + 256 * mMemory[BC_REG_IP + 1];

		mIP++;
		switch (d.mMode)
		{
			case ASMIM_IMPLIED:
				if (trace & 2)
					printf("%04x : %04x %02x __ __ %s         (A:%02x X:%02x Y:%02x P:%02x S:%02x)\n", iip, ip, mMemory[ip], AsmInstructionNames[d.mType], mRegA, mRegX, mRegY, mRegP, mRegS);
				mCycles[ip] += 2;
				break;
			case ASMIM_IMMEDIATE:
				addr = mMemory[mIP++];
				if (trace & 2)
					printf("%04x : %04x %02x %02x __ %s #$%02x    (A:%02x X:%02x Y:%02x P:%02x S:%02x)\n", iip, ip, mMemory[ip], mMemory[ip+1], AsmInstructionNames[d.mType], addr, mRegA, mRegX, mRegY, mRegP, mRegS);
				mCycles[ip] += 2;
				break;
			case ASMIM_ZERO_PAGE:
				addr = mMemory[mIP++];
				if (trace & 2)
					printf("%04x : %04x %02x %02x __ %s $%02x     (A:%02x X:%02x Y:%02x P:%02x S:%02x)\n", iip, ip, mMemory[ip], mMemory[ip + 1], AsmInstructionNames[d.mType], addr, mRegA, mRegX, mRegY, mRegP, mRegS);
				mCycles[ip] += 3;
				break;
			case ASMIM_ZERO_PAGE_X:
				taddr = mMemory[mIP++];
				addr = (taddr + mRegX) & 0xff;
				if (trace & 2)
					printf("%04x : %04x %02x %02x __ %s $%02x,x   (A:%02x X:%02x Y:%02x P:%02x S:%02x %04x)\n", iip, ip, mMemory[ip], mMemory[ip + 1], AsmInstructionNames[d.mType], taddr, mRegA, mRegX, mRegY, mRegP, mRegS, addr);
				mCycles[ip] += 3;
				break;
			case ASMIM_ZERO_PAGE_Y:
				taddr = mMemory[mIP++];
				addr = (taddr + mRegY) & 0xff;
				if (trace & 2)
					printf("%04x : %04x %02x %02x __ %s $%02x,y   (A:%02x X:%02x Y:%02x P:%02x S:%02x %04x)\n", iip, ip, mMemory[ip], mMemory[ip + 1], AsmInstructionNames[d.mType], taddr, mRegA, mRegX, mRegY, mRegP, mRegS, addr);
				mCycles[ip] += 3;
				break;
			case ASMIM_ABSOLUTE:
				addr = mMemory[mIP] + 256 * mMemory[mIP + 1];
				if (trace & 2)
					printf("%04x : %04x %02x %02x %02x %s $%04x   (A:%02x X:%02x Y:%02x P:%02x S:%02x)\n", iip, ip, mMemory[ip], mMemory[ip + 1], mMemory[ip + 2], AsmInstructionNames[d.mType], addr, mRegA, mRegX, mRegY, mRegP, mRegS);
				mIP += 2;
				mCycles[ip] += 4;
				break;
			case ASMIM_ABSOLUTE_X:
				taddr = mMemory[mIP] + 256 * mMemory[mIP + 1];
				addr = (taddr + mRegX) & 0xffff;
				if (trace & 2)
					printf("%04x : %04x %02x %02x %02x %s $%04x,x (A:%02x X:%02x Y:%02x P:%02x S:%02x %04x)\n", iip, ip, mMemory[ip], mMemory[ip + 1], mMemory[ip + 2], AsmInstructionNames[d.mType], taddr, mRegA, mRegX, mRegY, mRegP, mRegS, addr);
				mIP += 2;
				mCycles[ip] += 5;
				break;
			case ASMIM_ABSOLUTE_Y:
				taddr = mMemory[mIP] + 256 * mMemory[mIP + 1];
				addr = (taddr + mRegY) & 0xffff;
				if (trace & 2)
					printf("%04x : %04x %02x %02x %02x %s $%04x,y (A:%02x X:%02x Y:%02x P:%02x S:%02x %04x)\n", iip, ip, mMemory[ip], mMemory[ip + 1], mMemory[ip + 2], AsmInstructionNames[d.mType], taddr, mRegA, mRegX, mRegY, mRegP, mRegS, addr);
				mIP += 2;
				mCycles[ip] += 5;
				break;
			case ASMIM_INDIRECT:
				taddr = mMemory[mIP] + 256 * mMemory[mIP + 1];
				mIP += 2;
				addr = mMemory[taddr] + 256 * mMemory[taddr + 1];
				if (trace & 2)
					printf("%04x : %04x %02x %02x %02x %s ($%04x) (A:%02x X:%02x Y:%02x P:%02x S:%02x %04x)\n", iip, ip, mMemory[ip], mMemory[ip + 1], mMemory[ip + 2], AsmInstructionNames[d.mType], taddr, mRegA, mRegX, mRegY, mRegP, mRegS, addr);
				mCycles[ip] += 6;
				break;
			case ASMIM_INDIRECT_X:
				taddr = (mMemory[mIP++] + mRegX) & 0xff;
				addr = mMemory[taddr] + 256 * mMemory[taddr + 1];
				if (trace & 2)
					printf("%04x : %04x %02x %02x __ %s ($%02x,x) (A:%02x X:%02x Y:%02x P:%02x S:%02x %02x %04x)\n", iip, ip, mMemory[ip], mMemory[ip + 1], AsmInstructionNames[d.mType], mMemory[ip + 1], mRegA, mRegX, mRegY, mRegP, mRegS, taddr, addr);
				mCycles[ip] += 5;
				break;
			case ASMIM_INDIRECT_Y:
				taddr = mMemory[mIP++];
				addr = (mMemory[taddr] + 256 * mMemory[taddr + 1] + mRegY) & 0xffff;
				if (trace & 2)
					printf("%04x : %04x %02x %02x __ %s ($%02x),y (A:%02x X:%02x Y:%02x P:%02x S:%02x %04x)\n", iip, ip, mMemory[ip], mMemory[ip + 1], AsmInstructionNames[d.mType], taddr, mRegA, mRegX, mRegY, mRegP, mRegS, addr);
				mCycles[ip] += 4;
				break;
			case ASMIM_RELATIVE:
				taddr = mMemory[mIP++];
				if (taddr & 0x80)
					addr = taddr + mIP - 256;
				else
					addr = taddr + mIP;
				if (trace & 2)
					printf("%04x : %04x %02x %02x __ %s $%02x     (A:%02x X:%02x Y:%02x P:%02x S:%02x %04x)\n", iip, ip, mMemory[ip], mMemory[ip + 1], AsmInstructionNames[d.mType], taddr, mRegA, mRegX, mRegY, mRegP, mRegS, addr);
				mCycles[ip] += 2;
				break;
		}

		if ((trace & 1) && ip == 0x0823)
		{
			int	accu = mMemory[BC_REG_ACCU] + 256 * mMemory[BC_REG_ACCU + 1];
			int	ptr = mMemory[BC_REG_ADDR] + 256 * mMemory[BC_REG_ADDR + 1];
			int	sp = mMemory[BC_REG_STACK] + 256 * mMemory[BC_REG_STACK + 1];
			printf("%04x  (A:%04x P:%04x S:%04x) %04x %04x %04x %04x  %04x %04x %04x %04x\n", addr, accu, ptr, sp,
				mMemory[BC_REG_TMP +  0] + 256 * mMemory[BC_REG_TMP +  1],
				mMemory[BC_REG_TMP +  2] + 256 * mMemory[BC_REG_TMP +  3],
				mMemory[BC_REG_TMP +  4] + 256 * mMemory[BC_REG_TMP +  5],
				mMemory[BC_REG_TMP +  6] + 256 * mMemory[BC_REG_TMP +  7],
				mMemory[BC_REG_TMP +  8] + 256 * mMemory[BC_REG_TMP +  9],
				mMemory[BC_REG_TMP + 10] + 256 * mMemory[BC_REG_TMP + 11],
				mMemory[BC_REG_TMP + 12] + 256 * mMemory[BC_REG_TMP + 13],
				mMemory[BC_REG_TMP + 14] + 256 * mMemory[BC_REG_TMP + 15]
			);
		}

		if (!EmulateInstruction(d.mType, d.mMode, addr, mCycles[ip]))
			return -1;
	}

	if (mRegS == 0)
	{
		DumpCycles();
		return int16(mMemory[BC_REG_ACCU] + 256 * mMemory[BC_REG_ACCU + 1]);
	}

	return -1;
}