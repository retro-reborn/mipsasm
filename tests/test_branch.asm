# Branch and Jump Test
# This test covers various branches and jumps

.data
msg_start:  .asciiz "Branch Test: Starting...\n"
msg_pass:   .asciiz "Branch Test: PASSED\n"
msg_fail:   .asciiz "Branch Test: FAILED\n"

.text
main:
    # Print start message
    li      $v0, 4              # Print string syscall
    la      $a0, msg_start      # Load address of start message
    syscall
    # Initialize registers
    li      $t0, 10             # Counter
    li      $t1, 0              # Sum
    li      $t2, 55             # Expected result
    
loop:
    add     $t1, $t1, $t0       # Add counter to sum
    sub     $t0, $t0, 1         # Decrement counter
    bnez    $t0, loop           # Branch back if counter > 0
    
    # Test branches
    beq     $t1, $t2, test1     # Branch if sum equals expected
    li      $v0, 1              # Error code: sum calculation error
    j       exit                # Jump to exit
    
test1:
    li      $t3, 10             # Test value
    li      $t4, 20             # Comparison value
    
    # Test branch not equal
    bne     $t3, $t4, test2     # Branch if t3 != t4
    li      $v0, 2              # Error code: BNE failed
    j       exit                # Jump to exit
    
test2:
    li      $t5, 30             # Test value
    
    # Test branch less than (using slt and bne)
    slt     $t6, $t3, $t5       # Set t6 to 1 if t3 < t5
    beq     $t6, $zero, test3_fail
    
    # Test jump register
    la      $t9, test3          # Load address of test3
    jr      $t9                 # Jump to register
    
test3_fail:
    li      $v0, 3              # Error code: SLT test failed
    j       exit                # Jump to exit
    
test3:
    # Test jump and link
    jal     subroutine          # Jump and link
    
    # Test result from subroutine
    bne     $t7, $t8, fail      # Branch if not equal
    j       pass                # Jump to pass
    
subroutine:
    li      $t7, 123            # Set test value
    li      $t8, 123            # Expected value
    jr      $ra                 # Return
    
fail:
    li      $v0, 4              # Print string syscall
    la      $a0, msg_fail       # Load address of fail message
    syscall
    li      $v0, 4              # Error code: subroutine test failed
    j       exit                # Jump to exit
    
pass:
    li      $v0, 4              # Print string syscall
    la      $a0, msg_pass       # Load address of pass message
    syscall
    li      $v0, 0              # Success code
    
exit:
    # Exit program
    li      $v0, 10
    syscall
