# Test file demonstrating text and data sections
.text
.org 0x00400000   # Standard MIPS text start address

main:
    # Load addresses of data items
    la $t0, message
    la $t1, numbers
    
    # Load values from data section
    lw $t2, 0($t1)   # Load first number
    lw $t3, 4($t1)   # Load second number
    
    # Add the numbers
    add $t4, $t2, $t3
    
    # Store result back to memory
    sw $t4, 8($t1)   # Store at result location
    
    # Exit program
    li $v0, 10       # syscall code for exit
    syscall

.data
.org 0x10010000   # Standard MIPS data start address

message: .asciiz "Hello, MIPS world!"
numbers: .word 42, 58, 0   # Two values and space for result
