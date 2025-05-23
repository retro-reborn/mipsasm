# MIPS Assembler
A MIPS assembly language compiler that compiles MIPS assembly code into binary format.

## Features
- Supports a wide range of MIPS instructions including:
  - R-type instructions (ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, etc.)
  - I-type instructions (ADDI, ADDIU, ANDI, ORI, XORI, LW, SW, BEQ, BNE, etc.)
  - J-type instructions (J, JAL)
  - Pseudo-instructions (LI, LA, MOVE, etc.)
- Two-pass assembly for resolving labels
- Supports common assembler directives (.word, .byte, .half, .space, .align, .ascii, .asciiz)
- Support for symbolic labels
- Comprehensive error checking

## Building
To build the assembler, run:
```bash
make
```

This will create the `mipsasm` executable in the `bin` directory.

## Usage
```
Usage: mipsasm [options] input_file [output_file]
Options:
  -h, --help         Show this help message
  -o <file>          Specify output file
  -v, --verbose      Enable verbose output
```

### Examples
Assemble a MIPS source file:

```bash
./bin/mipsasm tests/test_basic.asm
```

## Supported Instructions

### R-type Instructions
- `add $rd, $rs, $rt` - Add
- `sub $rd, $rs, $rt` - Subtract
- `and $rd, $rs, $rt` - Bitwise AND
- `or $rd, $rs, $rt` - Bitwise OR
- `xor $rd, $rs, $rt` - Bitwise XOR
- `nor $rd, $rs, $rt` - Bitwise NOR
- `slt $rd, $rs, $rt` - Set Less Than
- `sltu $rd, $rs, $rt` - Set Less Than Unsigned
- `sll $rd, $rt, shamt` - Shift Left Logical
- `srl $rd, $rt, shamt` - Shift Right Logical
- `sra $rd, $rt, shamt` - Shift Right Arithmetic
- `sllv $rd, $rt, $rs` - Shift Left Logical Variable
- `srlv $rd, $rt, $rs` - Shift Right Logical Variable
- `srav $rd, $rt, $rs` - Shift Right Arithmetic Variable
- `jr $rs` - Jump Register
- `jalr $rs` or `jalr $rd, $rs` - Jump and Link Register
- `syscall` - System Call
- `break` - Break
- `mult $rs, $rt` - Multiply
- `multu $rs, $rt` - Multiply Unsigned
- `div $rs, $rt` - Divide
- `divu $rs, $rt` - Divide Unsigned
- `mfhi $rd` - Move From HI
- `mflo $rd` - Move From LO
- `mthi $rs` - Move To HI
- `mtlo $rs` - Move To LO

### I-type Instructions
- `addi $rt, $rs, imm` - Add Immediate
- `addiu $rt, $rs, imm` - Add Immediate Unsigned
- `andi $rt, $rs, imm` - AND Immediate
- `ori $rt, $rs, imm` - OR Immediate
- `xori $rt, $rs, imm` - XOR Immediate
- `slti $rt, $rs, imm` - Set Less Than Immediate
- `sltiu $rt, $rs, imm` - Set Less Than Immediate Unsigned
- `lw $rt, offset($rs)` - Load Word
- `sw $rt, offset($rs)` - Store Word
- `lb $rt, offset($rs)` - Load Byte
- `lbu $rt, offset($rs)` - Load Byte Unsigned
- `lh $rt, offset($rs)` - Load Halfword
- `lhu $rt, offset($rs)` - Load Halfword Unsigned
- `sb $rt, offset($rs)` - Store Byte
- `sh $rt, offset($rs)` - Store Halfword
- `beq $rs, $rt, label` - Branch on Equal
- `bne $rs, $rt, label` - Branch on Not Equal
- `lui $rt, imm` - Load Upper Immediate

### J-type Instructions
- `j label` - Jump
- `jal label` - Jump and Link

### Pseudo-instructions
- `li $rt, imm` - Load Immediate (expands to lui/ori as needed)
- `la $rt, label` - Load Address (expands to lui/ori as needed)
- `move $rd, $rs` - Move Register (implemented as addu $rd, $rs, $zero)
- `b label` - Branch (implemented as beq $zero, $zero, label)
- `beqz $rs, label` - Branch on Equal to Zero (implemented as beq $rs, $zero, label)
- `bnez $rs, label` - Branch on Not Equal to Zero (implemented as bne $rs, $zero, label)
- `nop` - No Operation (implemented as sll $zero, $zero, 0)

## Directives
- `.word value1, value2, ...` - Store words (32-bit values)
- `.half value1, value2, ...` - Store halfwords (16-bit values)
- `.byte value1, value2, ...` - Store bytes (8-bit values)
- `.ascii "string"` - Store ASCII string
- `.asciiz "string"` - Store ASCII string with null terminator
- `.space size` - Reserve space
- `.align power_of_2` - Align to power of 2 boundary

## Running Tests
To run the test suite:
```bash
make test
```

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
