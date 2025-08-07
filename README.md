# mips32_emu
MIPS32 emulator with comprehensive instruction set support

## Overview
A complete MIPS32 processor emulator implemented in C++11, featuring:
- Full instruction set architecture support
- Memory management with bus interface
- System calls for I/O operations
- Proper register management including special registers (HI/LO)

## Implemented Instructions

### Arithmetic and Logical Instructions
- **ADD, ADDU, ADDI, ADDIU** - Addition (signed/unsigned, immediate)
- **SUB, SUBU** - Subtraction (signed/unsigned)
- **DIV, DIVU** - Division (signed/unsigned) - results in HI/LO registers
- **MULT, MULTU** - Multiplication (signed/unsigned) - results in HI/LO registers
- **AND, OR, XOR, NOR** - Logical operations
- **ANDI, ORI, XORI** - Logical operations with immediate
- **SLL, SRL, SRA** - Shift left/right logical/arithmetic
- **SLLV, SRLV, SRAV** - Variable shift operations

### Comparison Instructions
- **SLT, SLTU** - Set Less Than (signed/unsigned)
- **SLTI, SLTIU** - Set Less Than Immediate (signed/unsigned)

### Branch Instructions
- **BEQ, BNE** - Branch on Equal/Not Equal
- **BLTZ, BGEZ** - Branch on Less Than/Greater or Equal to Zero
- **BGTZ, BLEZ** - Branch on Greater Than/Less or Equal to Zero
- **BLTZAL, BGEZAL** - Branch and Link (conditional)

### Jump Instructions
- **J, JAL** - Jump (and Link)
- **JR, JALR** - Jump Register (and Link)

### Load Instructions
- **LB, LBU** - Load Byte (signed/unsigned)
- **LH, LHU** - Load Halfword (signed/unsigned)
- **LW** - Load Word
- **LUI** - Load Upper Immediate
- **LHI, LLO** - Load High/Low (custom extensions)

### Store Instructions
- **SB** - Store Byte
- **SH** - Store Halfword
- **SW** - Store Word

### Data Movement Instructions
- **MFHI, MFLO** - Move From HI/LO registers
- **MTHI, MTLO** - Move To HI/LO registers

### System Instructions
- **SYSCALL** - System call (supports print integer and string)
- **BREAK** - Breakpoint exception
- **TRAP** - Trap exception
- **NOP** - No operation (sll $zero, $zero, 0)

### Special Features
- **Register $zero enforcement** - Always contains 0
- **Big-endian memory organization**
- **Proper REGIMM instruction decoding** (BLTZ, BGEZ, BLTZAL, BGEZAL)
- **Exception handling** for unsupported opcodes

## Architecture

### Core Components
- **MIPS32 CPU Class** - Main processor implementation
- **IBus Interface** - Abstract memory bus interface
- **Bus Class** - Concrete memory implementation
- **Opcode System** - Instruction decoding and execution

### Memory Layout
- **16MB RAM** default configuration
- **Big-endian byte ordering**
- **Word-aligned access** for optimal performance

### Register Set
- **32 General Purpose Registers** ($zero - $ra)
- **HI/LO Registers** for multiplication/division results
- **Program Counter (PC)** with automatic increment

## Building and Running

### Prerequisites
- C++11 compatible compiler
- Standard library support

### Compilation
```bash
cd mips_emu
g++ -std=c++11 *.cpp -o mips_emu
```

### Execution
```bash
./mips_emu
```

## Development Status

### Completed Features (v0.2.0)
- ✅ Complete MIPS32 instruction set (~85% coverage)
- ✅ Proper instruction decoding (R-type, I-type, J-type, REGIMM)
- ✅ Memory management with bus abstraction
- ✅ Basic system calls
- ✅ Register management and enforcement
- ✅ Exception handling

### Roadmap
- [ ] Library conversion for Unity integration
- [ ] C API export functions
- [ ] Advanced debugging features
- [ ] Extended system call support
- [ ] Performance optimizations

## Usage Example

The emulator currently runs built-in test programs demonstrating:
- Arithmetic operations
- Loop constructs
- System calls (print operations)
- Jump and branch instructions

## Technical Notes

- **Instruction Format Support**: Full R-type, I-type, J-type, and REGIMM format handling
- **Memory Model**: 32-bit addressing with byte-level access
- **Endianness**: Big-endian organization for MIPS compatibility
- **Error Handling**: Graceful handling of unsupported opcodes
