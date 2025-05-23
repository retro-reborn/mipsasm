# Arithmetic Operations Test
# This test covers various arithmetic and logical operations

.data
msg_start:  .asciiz "Arithmetic Test: Starting...\n"
msg_result: .asciiz "Arithmetic Test: Result = "
msg_pass:   .asciiz "Arithmetic Test: PASSED\n"
newline:    .asciiz "\n"

.text
main:
    # Print start message
    li      $v0, 4              # Print string syscall
    la      $a0, msg_start      # Load address of start message
    syscall
    # Initialize registers
    li      $t0, 100            # First operand
    li      $t1, 50             # Second operand
    li      $t2, 0xAAAAAAAA     # Bit pattern 1
    li      $t3, 0x55555555     # Bit pattern 2
    
    # Arithmetic operations
    add     $s0, $t0, $t1       # 100 + 50 = 150
    sub     $s1, $t0, $t1       # 100 - 50 = 50
    addi    $s2, $t0, 25        # 100 + 25 = 125
    
    # Logical operations
    and     $s3, $t2, $t3       # Bitwise AND = 0
    or      $s4, $t2, $t3       # Bitwise OR = 0xFFFFFFFF
    xor     $s5, $t2, $t3       # Bitwise XOR = 0xFFFFFFFF
    nor     $s6, $t2, $t3       # Bitwise NOR = 0
    
    # Immediate logical operations
    andi    $s7, $t2, 0xF0F0    # Bitwise AND immediate
    ori     $t4, $t3, 0xF0F0    # Bitwise OR immediate
    xori    $t5, $t2, 0xFFFF    # Bitwise XOR immediate
    
    # Shifts
    sll     $t6, $t0, 2         # 100 << 2 = 400
    srl     $t7, $t0, 2         # 100 >> 2 = 25
    
    # Multiplication and division (usually done in HI/LO registers)
    mult    $t0, $t1            # 100 * 50 = 5000
    mflo    $s0                 # Get LO result
    
    div     $t0, $t1            # 100 / 50 = 2
    mflo    $s1                 # Get quotient
    mfhi    $s2                 # Get remainder (should be 0)
    
    # Print some results
    li      $v0, 4              # Print string syscall
    la      $a0, msg_result     # Load message
    syscall
    
    li      $v0, 1              # Print integer syscall
    move    $a0, $s1            # Print quotient (should be 2)
    syscall
    
    li      $v0, 4              # Print string syscall
    la      $a0, newline        # Print newline
    syscall
    
    # Print success message
    li      $v0, 4              # Print string syscall
    la      $a0, msg_pass       # Load address of pass message
    syscall
    
    # Exit program
    li      $v0, 10
    syscall
