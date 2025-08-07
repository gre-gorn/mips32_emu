#include "mips32.h"

#include <algorithm>
#include <iostream>
#include <bitset>

using namespace std;

void MIPS32::SetPC(uint32_t addr)
{
	_pc = addr;
}

uint32_t MIPS32::GetPC()
{
	return _pc;
}

void MIPS32::EnableLog(bool enable)
{
	_logEnabled = enable;
}

bool MIPS32::Tick()
{
	Fetch();
	if (_fetched == (uint32_t)0x0)
		NOP();
	else
		Execute();

	// Ensure register 0 is always zero
	_registers[0] = 0;

	_clock++;

	return _break;
}

void MIPS32::Fetch()
{
	if (_bus != nullptr)
	{
		_fetched = _bus->Read(_pc);
		_pc += 4;
		if (_pc > _bus->Size())
		{
			cout << "PC overturned" << endl;
			_pc = 0; // overturn _pc when we reach end of memory
		}
	}
}

void MIPS32::Execute()
{
	Decode();
	(this->*_opcode.func)(); // call the function
}

void MIPS32::Decode()
{
	uint32_t opcode = _fetched & OPCODE_MASK;
	uint32_t funct = _fetched & FUNCT_MASK;
	uint32_t rt = RT(_fetched);

	// check if it's R-Type
	// R-Type commands op = 0 always
	if (opcode == 0)
	{
		for (vector<Opcode>::iterator i = _ropcodes.begin(); i != _ropcodes.end(); i++)
		{
			if (opcode == 0 && funct == ((Opcode)*i).inst)
			{
				_opcode = *i;
				return;
			}
		}
	}

	// check if it's REGIMM (opcode 1) - uses RT field to determine instruction
	if (opcode == (0b000001 << 26))
	{
		switch (rt)
		{
		case 0b00000: // BLTZ
			_opcode.mnemonic = "bltz";
			_opcode.inst = (uint32_t)0b000001 << 26;
			_opcode.func = &MIPS32::BLTZ;
			return;
		case 0b00001: // BGEZ
			_opcode.mnemonic = "bgez";
			_opcode.inst = (uint32_t)0b000001 << 26;
			_opcode.func = &MIPS32::BGEZ;
			return;
		case 0b10000: // BLTZAL
			_opcode.mnemonic = "bltzal";
			_opcode.inst = (uint32_t)0b000001 << 26;
			_opcode.func = &MIPS32::BLTZAL;
			return;
		case 0b10001: // BGEZAL
			_opcode.mnemonic = "bgezal";
			_opcode.inst = (uint32_t)0b000001 << 26;
			_opcode.func = &MIPS32::BGEZAL;
			return;
		}
	}

	// check if it's J-Type
	for (vector<Opcode>::iterator i = _jopcodes.begin(); i != _jopcodes.end(); i++)
	{
		if (((Opcode)*i).inst == opcode)
		{
			_opcode = *i;
			return;
		}
	}

	// if it's not R-Type nor J-Type nor REGIMM
	// then it should be I-Type
	for (vector<Opcode>::iterator i = _iopcodes.begin(); i != _iopcodes.end(); i++)
	{
		if (((Opcode)*i).inst == opcode)
		{
			_opcode = *i;
			return;
		}
	}

	// unsupported opcode
	printf("usupported opcode %x\n", _fetched);
	cout << bitset<32>{_fetched} << endl;
	_break = true;
}

// based on:
// https://s3-eu-west-1.amazonaws.com/downloads-mips/documents/MD00086-2B-MIPS32BIS-AFP-6.06.pdf
void MIPS32::InitOpcodes()
{
	_ropcodes = {
		// R-Format
		{"sll", (uint32_t)0b000000, &MIPS32::SLL},	 // 0
		{"srl", (uint32_t)0b000010, &MIPS32::SRL},	 // 2, unsigned right shift
		{"sra", (uint32_t)0b000011, &MIPS32::SRA},	 // 3, signed right shift
		{"sllv", (uint32_t)0b000100, &MIPS32::SLLV}, // 4
		{"srlv", (uint32_t)0b000110, &MIPS32::SRLV}, // 6, unsigned right shift
		{"srav", (uint32_t)0b000111, &MIPS32::SRAV}, // 7, signed right shift
		{"jr", (uint32_t)0b001000, &MIPS32::JR},	 // 8, R[$rs] must be a multiple of 4

		// can have special of jalr $rs form when $rd == 31
		{"jalr", (uint32_t)0b001001, &MIPS32::JALR}, // 9, R[$rs] must be a multiple of 4

		{"syscall", (uint32_t)0b001100, &MIPS32::SYSCALL}, // 12
		{"break", (uint32_t)0b001101, &MIPS32::BREAK},	   // 13
		{"mfhi", (uint32_t)0b010000, &MIPS32::MFHI},	   // 16
		{"mthi", (uint32_t)0b010001, &MIPS32::MTHI},	   // 17
		{"mflo", (uint32_t)0b010010, &MIPS32::MFLO},	   // 18
		{"mtlo", (uint32_t)0b010011, &MIPS32::MTLO},	   // 19
		{"mult", (uint32_t)0b011000, &MIPS32::MULT},	   // 24, signed multiplication
		{"multu", (uint32_t)0b011001, &MIPS32::MULTU},	   // 25, unsigned multiplication
		{"div", (uint32_t)0b011010, &MIPS32::DIV},		   // 26, signed division
		{"divu", (uint32_t)0b011011, &MIPS32::DIVU},	   // 27, unsigned division
		{"add", (uint32_t)0b100000, &MIPS32::ADD},		   // 32, exception on signed overflow
		{"addu", (uint32_t)0b100001, &MIPS32::ADDU},	   // 33
		{"sub", (uint32_t)0b100010, &MIPS32::SUB},		   // 34, exception on signed overflow
		{"subu", (uint32_t)0b100011, &MIPS32::SUBU},	   // 35
		{"and", (uint32_t)0b100100, &MIPS32::AND},		   // 36
		{"or", (uint32_t)0b100101, &MIPS32::OR},		   // 37
		{"xor", (uint32_t)0b100110, &MIPS32::XOR},		   // 38
		{"nor", (uint32_t)0b100111, &MIPS32::NOR},		   // 39
		{"slt", (uint32_t)0b101010, &MIPS32::SLT},		   // 42, signed comparison
		{"sltu", (uint32_t)0b101011, &MIPS32::SLTU},	   // 43, unsigned comparison
	};

	_jopcodes = {
		// J-Format
		// Jump instructions use pseudo-absolute addressing, in which the upper 4 bits
		// of the computed address are taken relatively from the program counter.
		{"j", (uint32_t)0b000010 << 26, &MIPS32::J},	 // 2,
		{"jal", (uint32_t)0b000011 << 26, &MIPS32::JAL}, // 3,
	};

	_iopcodes = {
		// I-Format
		// These instructions are identified and differentiated by their opcode numbers
		//(any number greater than 3). All of these instructions feature a 16-bit immediate,
		// which is sign-extended to a 32-bit value in every instruction
		//(except for the and, or, and xor instructions which zero-extend
		// and the lui instruction in which it does not matter).
		// Branch instructions also effectively multiply the immediate by 4, to get a byte offset.

		// Note: REGIMM instructions (opcode 1) are handled separately in Decode()

		{"beq", (uint32_t)0b000100 << 26, &MIPS32::BEQ},	 // 4, branch on equal
		{"bne", (uint32_t)0b000101 << 26, &MIPS32::BNE},	 // 5, branch on not equal
		{"blez", (uint32_t)0b000110 << 26, &MIPS32::BLEZ},	 // 6, signed comparison, <= 0
		{"bgtz", (uint32_t)0b000111 << 26, &MIPS32::BGTZ},	 // 7, signed comparison, > 0
		{"addi", (uint32_t)0b001000 << 26, &MIPS32::ADDI},	 // 8, exception on signed overflow
		{"addiu", (uint32_t)0b001001 << 26, &MIPS32::ADDIU}, // 9
		{"slti", (uint32_t)0b001010 << 26, &MIPS32::SLTI},	 // 10, signed comparison
		{"sltiu", (uint32_t)0b001011 << 26, &MIPS32::SLTIU}, // 11, unsigned comparison
		{"andi", (uint32_t)0b001100 << 26, &MIPS32::ANDI},	 // 12
		{"ori", (uint32_t)0b001101 << 26, &MIPS32::ORI},	 // 13
		{"xori", (uint32_t)0b001110 << 26, &MIPS32::XORI},	 // 14
		{"lui", (uint32_t)0b001111 << 26, &MIPS32::LUI},	 // 15

		{"lb", (uint32_t)0b100000 << 26, &MIPS32::LB},	 // 32
		{"lh", (uint32_t)0b100001 << 26, &MIPS32::LH},	 // 33, computed address must be a multiple of 2
		{"lw", (uint32_t)0b100011 << 26, &MIPS32::LW},	 // 34, computed address must be a multiple of 4
		{"lbu", (uint32_t)0b100100 << 26, &MIPS32::LBU}, // 36
		{"lhu", (uint32_t)0b100101 << 26, &MIPS32::LHU}, // 37, computed address must be a multiple of 2
		{"sb", (uint32_t)0b101000 << 26, &MIPS32::SB},	 // 40
		{"sh", (uint32_t)0b101001 << 26, &MIPS32::SH},	 // 41, computed address must be a multiple of 2
		{"sw", (uint32_t)0b101011 << 26, &MIPS32::SW},	 // 43, computed address must be a multiple of 4

		/*
		 * those 2 below found here:
		 * http://alumni.cs.ucr.edu/~vladimir/cs161/mips.html
		 * https://student.cs.uwaterloo.ca/~isg/res/mips/opcodes
		 * https://uweb.engr.arizona.edu/~ece369/Resources/spim/MIPSReference.pdf
		 */
		{"llo", (uint32_t)0b011000 << 26, &MIPS32::LLO}, // 24
		{"lhi", (uint32_t)0b011001 << 26, &MIPS32::LHI}, // 25

		// Exception and interrupt instructions
		// https://student.cs.uwaterloo.ca/~isg/res/mips/traps
		{"trap", (uint32_t)0b011010 << 26, &MIPS32::TRAP},
	};

	cout << "Opcodes initialized" << endl;
}

MIPS32::MIPS32(IBus *bus)
{
	InitOpcodes();
	_clock = 0;
	_bus = bus;
	memset(_registers, 0, sizeof(_registers));
}

void MIPS32::LogShift(bool printRT = false)
{
	if (!_logEnabled)
		return;

	cout << _opcode.mnemonic;
	cout << " " << _reg_names[(Reg)RT(_fetched)];
	cout << ", " << _reg_names[(Reg)RD(_fetched)];
	cout << ", " << hex << SHAMT(_fetched);
	if (printRT)
	{
		cout << " => " << _reg_names[(Reg)RT(_fetched)] << " = " << hex << "0x" << _registers[RT(_fetched)];
	}
	cout << endl;
}

void MIPS32::LogImm(bool printRT = false)
{
	if (!_logEnabled)
		return;

	cout << _opcode.mnemonic;
	cout << " " << _reg_names[(Reg)RT(_fetched)];
	cout << ", " << _reg_names[(Reg)RS(_fetched)];
	cout << ", " << hex << "0x" << IMM(_fetched);
	if (printRT)
	{
		cout << " => " << _reg_names[(Reg)RT(_fetched)] << " = " << hex << "0x" << _registers[RT(_fetched)];
	}
	cout << endl;
}

void MIPS32::LogReg(bool printRD = false)
{
	if (!_logEnabled)
		return;

	cout << _opcode.mnemonic;
	cout << " " << _reg_names[(Reg)RD(_fetched)];
	cout << ", " << _reg_names[(Reg)RS(_fetched)];
	cout << ", " << _reg_names[(Reg)RT(_fetched)];
	if (printRD)
	{
		cout << " => " << _reg_names[(Reg)RD(_fetched)] << " = " << hex << "0x" << _registers[RD(_fetched)];
	}
	cout << endl;
}

void MIPS32::LogJr(bool printRA = false)
{
	if (!_logEnabled)
		return;

	cout << _opcode.mnemonic;
	cout << " " << _reg_names[(Reg)RS(_fetched)];
	if (printRA)
	{
		cout << " => " << _reg_names[(Reg)RS(_fetched)] << " = " << hex << "0x" << _registers[RS(_fetched)];
	}
	cout << endl;
}

void MIPS32::LogJ()
{
	if (!_logEnabled)
		return;

	cout << _opcode.mnemonic;
	cout << " " << hex << "0x" << (uint32_t)((_pc & J_MASK) | J_TARGET(_fetched));
	cout << endl;
}

// Arithmeticand logical instructions
void MIPS32::ADD()
{
	_registers[RD(_fetched)] = (int32_t)_registers[RS(_fetched)] + (int32_t)_registers[RT(_fetched)];
	LogReg(true);
}

void MIPS32::ADDI()
{
	_registers[RT(_fetched)] = (int32_t)_registers[(Reg)RS(_fetched)] + (int16_t)IMM(_fetched);
	LogImm(true);
}

void MIPS32::ADDIU()
{
	_registers[RT(_fetched)] = _registers[(Reg)RS(_fetched)] + IMM(_fetched);
	LogImm(true);
}

void MIPS32::ADDU()
{
	_registers[RD(_fetched)] = _registers[RS(_fetched)] + _registers[RT(_fetched)];
	LogReg(true);
}

void MIPS32::SUB()
{
	_registers[RD(_fetched)] = (int32_t)_registers[RS(_fetched)] - (int32_t)_registers[RT(_fetched)];
	LogReg(true);
}

void MIPS32::SUBU()
{
	_registers[RD(_fetched)] = _registers[RS(_fetched)] - _registers[RT(_fetched)];
	LogReg(true);
}

void MIPS32::DIV()
{
	// hi - remainder
	// lo - quotient
	_registers[(Reg)hi] = (int32_t)_registers[RS(_fetched)] % (int32_t)_registers[RT(_fetched)];
	_registers[(Reg)lo] = (int32_t)_registers[RS(_fetched)] / (int32_t)_registers[RT(_fetched)];
	LogReg(true);
}

void MIPS32::DIVU()
{
	// hi - remainder
	// lo - quotient
	_registers[(Reg)hi] = _registers[RS(_fetched)] % _registers[RT(_fetched)];
	_registers[(Reg)lo] = _registers[RS(_fetched)] / _registers[RT(_fetched)];
	LogReg(true);
}

void MIPS32::MULT()
{
	int64_t result = (int64_t)((int32_t)_registers[RS(_fetched)] * (int32_t)_registers[RT(_fetched)]);
	_registers[(Reg)hi] = (int32_t)(result >> 32);
	_registers[(Reg)lo] = (int32_t)(result & 0x00000000ffffffff);
	LogReg(true);
}

void MIPS32::MULTU()
{
	uint64_t result = (uint64_t)(_registers[RS(_fetched)] * _registers[RT(_fetched)]);
	_registers[(Reg)hi] = (uint32_t)(result >> 32);
	_registers[(Reg)lo] = (uint32_t)(result & 0x00000000ffffffff);
	LogReg(true);
}

void MIPS32::AND()
{
	_registers[RD(_fetched)] = _registers[RS(_fetched)] & _registers[RT(_fetched)];
	LogReg(true);
}

void MIPS32::ANDI()
{
	_registers[RT(_fetched)] = _registers[(Reg)RS(_fetched)] & IMM(_fetched);
	LogImm(true);
}

void MIPS32::OR()
{
	_registers[RD(_fetched)] = _registers[RS(_fetched)] | _registers[RT(_fetched)];
	LogReg(true);
}

void MIPS32::ORI()
{
	_registers[RT(_fetched)] = _registers[(Reg)RS(_fetched)] | IMM(_fetched);
	LogImm(true);
}

void MIPS32::NOR()
{
	_registers[RD(_fetched)] = ~(_registers[RS(_fetched)] | _registers[RT(_fetched)]);
	LogReg(true);
}

void MIPS32::XOR()
{
	_registers[RD(_fetched)] = _registers[RS(_fetched)] ^ _registers[RT(_fetched)];
	LogReg(true);
}

void MIPS32::XORI()
{
	_registers[RT(_fetched)] = _registers[(Reg)RS(_fetched)] ^ IMM(_fetched);
	LogImm(true);
}

void MIPS32::SLL()
{
	_registers[(Reg)RT(_fetched)] = _registers[(Reg)RD(_fetched)] << SHAMT(_fetched);
	LogShift(true);
}

void MIPS32::SLLV()
{
	_registers[(Reg)RT(_fetched)] = _registers[(Reg)RD(_fetched)] << _registers[(Reg)RS(_fetched)];
	LogShift(true);
}

void MIPS32::SRAV()
{
	_registers[(Reg)RT(_fetched)] = (int32_t)_registers[(Reg)RD(_fetched)] >> _registers[(Reg)RS(_fetched)];
	LogShift(true);
}

void MIPS32::SRL()
{
	_registers[(Reg)RT(_fetched)] = _registers[(Reg)RD(_fetched)] >> SHAMT(_fetched);
	LogShift(true);
}

void MIPS32::SRA()
{
	_registers[(Reg)RT(_fetched)] = (int32_t)_registers[(Reg)RD(_fetched)] >> SHAMT(_fetched);
	LogShift(true);
}

void MIPS32::SRLV()
{
	_registers[(Reg)RT(_fetched)] = _registers[(Reg)RD(_fetched)] >> _registers[(Reg)RS(_fetched)];
	LogShift(true);
}

// Comparison instructions
void MIPS32::SLT()
{
	_registers[RD(_fetched)] = ((int32_t)_registers[RS(_fetched)] < (int32_t)_registers[RT(_fetched)]) ? 1 : 0;
	LogReg(true);
}

void MIPS32::SLTU()
{
	_registers[RD(_fetched)] = (_registers[RS(_fetched)] < _registers[RT(_fetched)]) ? 1 : 0;
	LogReg(true);
}

void MIPS32::SLTI()
{
	_registers[RT(_fetched)] = ((int32_t)_registers[RS(_fetched)] < (int16_t)IMM(_fetched)) ? 1 : 0;
	LogImm(true);
}

void MIPS32::SLTIU()
{
	_registers[RT(_fetched)] = (_registers[RS(_fetched)] < IMM(_fetched)) ? 1 : 0;
	LogImm(true);
}

// Branch instructions
void MIPS32::BEQ()
{
	bool branch = _registers[(Reg)RS(_fetched)] == _registers[(Reg)RT(_fetched)];
	// TODO: Tick();//branch delay
	_pc = branch ? (_pc + (IMM(_fetched) << 2)) : _pc;
	LogImm();
}

void MIPS32::BNE()
{
	bool branch = _registers[(Reg)RS(_fetched)] != _registers[(Reg)RT(_fetched)];
	// TODO: Tick();//branch delay
	_pc = branch ? (_pc + (IMM(_fetched) << 2)) : _pc;
	LogImm();
}

void MIPS32::BLTZ()
{
	bool branch = (int32_t)_registers[(Reg)RS(_fetched)] < 0;
	// TODO: Tick();//branch delay
	_pc = branch ? (_pc + (IMM(_fetched) << 2)) : _pc;
	LogImm();
}

void MIPS32::BGEZ()
{
	bool branch = (int32_t)_registers[(Reg)RS(_fetched)] >= 0;
	// TODO: Tick();//branch delay
	_pc = branch ? (_pc + (IMM(_fetched) << 2)) : _pc;
	LogImm();
}

void MIPS32::BGTZ()
{
	bool branch = (int32_t)_registers[(Reg)RS(_fetched)] > 0;
	// TODO: Tick();//branch delay
	_pc = branch ? (_pc + (IMM(_fetched) << 2)) : _pc;
	LogImm();
}

void MIPS32::BLEZ()
{
	bool branch = (int32_t)_registers[(Reg)RS(_fetched)] <= 0;
	// TODO: Tick();//branch delay
	_pc = branch ? (_pc + (IMM(_fetched) << 2)) : _pc;
	LogImm();
}

void MIPS32::BGEZAL()
{
	bool branch = (int32_t)_registers[(Reg)RS(_fetched)] >= 0;
	_registers[ra] = _pc; // Save return address
	// TODO: Tick();//branch delay
	_pc = branch ? (_pc + (IMM(_fetched) << 2)) : _pc;
	LogImm();
}

void MIPS32::BLTZAL()
{
	bool branch = (int32_t)_registers[(Reg)RS(_fetched)] < 0;
	_registers[ra] = _pc; // Save return address
	// TODO: Tick();//branch delay
	_pc = branch ? (_pc + (IMM(_fetched) << 2)) : _pc;
	LogImm();
}

// Breakpoint epc=pc; pc=0x3c	000000|code|001101
void MIPS32::BREAK()
{
	if (_logEnabled)
	{
		cout << _opcode.mnemonic << endl;
	}
	// Set breakpoint - halt execution
	_break = true;
}

// Jump instructions
void MIPS32::J()
{
	uint32_t j_target = (uint32_t)(_pc & J_MASK) | J_TARGET(_fetched);
	// TODO: Tick();//branch delay
	_pc = j_target;
	LogJ();
}

void MIPS32::JAL()
{
	_registers[ra] = _pc; // Save return address
	uint32_t j_target = (uint32_t)(_pc & J_MASK) | J_TARGET(_fetched);
	// TODO: Tick();//branch delay
	_pc = j_target;
	LogJ();
}

void MIPS32::JALR()
{
	uint32_t target = _registers[(Reg)RS(_fetched)];
	_registers[RD(_fetched)] = _pc; // Save return address (default $ra if RD not specified)
	// TODO: Tick();//branch delay
	_pc = target;
	LogJr(true);
}

void MIPS32::JR()
{
	uint32_t j_target = _registers[(Reg)RS(_fetched)];
	// TODO: Tick();//branch delay
	_pc = j_target;
	LogJr(true);
}

// Load instructions
void MIPS32::LHI()
{
	_registers[RT(_fetched)] = (IMM(_fetched) << 16) & HI_WORD_MASK;
	LogImm(true);
}

void MIPS32::LLO()
{
	_registers[RT(_fetched)] = IMM(_fetched) & LO_WORD_MASK;
	LogImm(true);
}

void MIPS32::LUI()
{
	_registers[RT(_fetched)] = IMM(_fetched) << 16;
	LogImm(true);
}

void MIPS32::LB()
{
	uint32_t addr = _registers[(Reg)RS(_fetched)] + (int16_t)IMM(_fetched);
	int8_t byte = (int8_t)((_bus->Read(addr) >> 24) & BYTE_MASK);
	_registers[(Reg)RT(_fetched)] = (int32_t)byte; // Sign extend
	LogImm(true);
}

void MIPS32::LBU()
{
	uint32_t addr = _registers[(Reg)RS(_fetched)] + (int16_t)IMM(_fetched);
	uint8_t byte = (_bus->Read(addr) >> 24) & BYTE_MASK;
	_registers[(Reg)RT(_fetched)] = (uint32_t)byte; // Zero extend
	LogImm(true);
}

void MIPS32::LH()
{
	uint32_t addr = _registers[(Reg)RS(_fetched)] + (int16_t)IMM(_fetched);
	uint16_t halfword = (_bus->Read(addr) >> 16) & 0xFFFF; // Load upper 16 bits
	_registers[(Reg)RT(_fetched)] = (int16_t)halfword;	   // Sign extend
	LogImm(true);
}

void MIPS32::LHU()
{
	uint32_t addr = _registers[(Reg)RS(_fetched)] + (int16_t)IMM(_fetched);
	uint16_t halfword = (_bus->Read(addr) >> 16) & 0xFFFF; // Load upper 16 bits
	_registers[(Reg)RT(_fetched)] = halfword;			   // Zero extend
	LogImm(true);
}

void MIPS32::LW()
{
	uint32_t addr = _registers[(Reg)RS(_fetched)] + (int16_t)IMM(_fetched);
	_registers[(Reg)RT(_fetched)] = _bus->Read(addr); // Load full word
	LogImm(true);
}

// Store instructions
void MIPS32::SB()
{
	uint32_t addr = _registers[(Reg)RS(_fetched)] + (int16_t)IMM(_fetched);
	uint8_t byte = _registers[(Reg)RT(_fetched)] & BYTE_MASK;
	_bus->Write(addr, byte);
	LogImm();
}

void MIPS32::SH()
{
	uint32_t addr = _registers[(Reg)RS(_fetched)] + (int16_t)IMM(_fetched);
	uint16_t halfword = _registers[(Reg)RT(_fetched)] & LO_WORD_MASK;
	// Store halfword - need to write 2 bytes
	_bus->Write(addr, (halfword >> 8) & 0xFF); // High byte
	_bus->Write(addr + 1, halfword & 0xFF);	   // Low byte
	LogImm();
}

void MIPS32::SW()
{
	uint32_t addr = _registers[(Reg)RS(_fetched)] + (int16_t)IMM(_fetched);
	uint32_t word = _registers[(Reg)RT(_fetched)];
	// Store word - need to write 4 bytes (big endian)
	_bus->Write(addr, (word >> 24) & 0xFF);		// Byte 3
	_bus->Write(addr + 1, (word >> 16) & 0xFF); // Byte 2
	_bus->Write(addr + 2, (word >> 8) & 0xFF);	// Byte 1
	_bus->Write(addr + 3, word & 0xFF);			// Byte 0
	LogImm();
}

// Data movement instructions
void MIPS32::MFHI()
{
	_registers[(Reg)RD(_fetched)] = _registers[(Reg)hi];
}

void MIPS32::MFLO()
{
	_registers[(Reg)RD(_fetched)] = _registers[(Reg)lo];
}

void MIPS32::MTHI()
{
	_registers[(Reg)hi] = _registers[(Reg)RS(_fetched)];
}

void MIPS32::MTLO()
{
	_registers[(Reg)lo] = _registers[(Reg)RS(_fetched)];
}

// Exception and interrupt instructions
void MIPS32::TRAP()
{
	if (_logEnabled)
	{
		cout << _opcode.mnemonic << endl;
	}
	// Trap instruction - halt execution with exception
	_break = true;
}

// Syscall
// System Call	epc=pc; pc=0x3c	000000|00000000000000000000|001100
void MIPS32::SYSCALL()
{
	if (_logEnabled)
	{
		cout << _opcode.mnemonic << endl;
	}

	switch (_registers[v0])
	{
	case 1: // print integer
	{
		cout << dec << _registers[a0];
	}
	break;

	case 4: // print string
	{
		string str = "";
		uint32_t index = 0;

		while (true)
		{
			uint8_t byte = (uint8_t)((_bus->Read(_registers[a0] + index) >> 24));
			if (byte == 0)
				break;
			str.push_back((char)byte);
			index += 1;
		};
		if (str.length() > 0)
			cout << str;
	}
	}
}

// Coprocessor
// Move From Coprocessor	rt=CPR[0,rd]	010000|00000|rt|rd|00000000000
void MIPS32::MFC0()
{
}

// Move To Coprocessor	CPR[0,rd]=rt	010000|00100|rt|rd|00000000000
void MIPS32::MTC0()
{
}

void MIPS32::NOP()
{
	if (_logEnabled)
	{
		cout << "nop" << endl;
	}
}
