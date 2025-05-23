# Basic MIPS Assembly Test
# This test covers basic instructions and pseudo-instructions

.data
msg_start:  .asciiz "Basic Test: Starting...\n"
msg_pass:   .asciiz "Basic Test: PASSED\n"

.text
main:
    # Print start message
    li      $v0, 4              # Print string syscall
    la      $a0, msg_start      # Load address of start message
    syscall
    # Register manipulation
    li      $t0, 10             # Load immediate
    li      $t1, 0xABCD1234     # Load large immediate
    move    $t2, $t0            # Move
    
    # Basic arithmetic
    addi    $t3, $t0, 5         # Add immediate
    addiu   $t4, $t0, 10        # Add immediate unsigned
    
    # Logical operations
    ori     $t5, $zero, 0xFF    # OR immediate
    andi    $t6, $t5, 0x0F      # AND immediate
    xori    $t7, $t5, 0xFF      # XOR immediate
    
    # R-type arithmetic
    add     $s0, $t0, $t1       # Add
    sub     $s1, $t1, $t0       # Subtract
    and     $s2, $t5, $t6       # AND
    or      $s3, $t5, $t6       # OR
    xor     $s4, $t5, $t6       # XOR
    
    # Load and store
    sw      $t0, 0($sp)         # Store word
    lw      $t8, 0($sp)         # Load word
    
    # Shifts
    sll     $s5, $t5, 4         # Shift left logical
    srl     $s6, $t5, 4         # Shift right logical
    
    # Branches and jumps
    beq     $t0, $t0, target    # Branch if equal
    nop                         # No operation

target:
    j       end                 # Jump
    nop                         # No operation
    
end:
    # Print success message
    li      $v0, 4              # Print string syscall
    la      $a0, msg_pass       # Load address of pass message
    syscall
    
    li      $v0, 10             # Exit syscall code
    syscall                     # System call
