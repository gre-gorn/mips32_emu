# mips32_emu
MIPS32 emulator

## implemented instructions

- ADD, ADDIU, ADDU
- SUB, SUBU
- DIV, DIVU, MULT, MULTU,
- SLL, SRL
- SLLV, SRAV, SRA, SRLV
- LHI, LB, LBU, LW
- MFHI, MFLO, MTHI, MTLO
- JR, J, BEQ, BNE, BLTZ, BGEZ, SRL
- SYSCALL (prints integer and asciiz string)
- NOP (in fact it is sll $zero, $zero, 0)
