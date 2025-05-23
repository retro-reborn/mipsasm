# Memory Operations Test
# This test covers various memory operations

.data
buffer:     .word   0, 0, 0, 0, 0         # 5 words of storage
byte_data:  .byte   0x12, 0x34, 0x56, 0x78 # 4 bytes of data
half_data:  .half   0xABCD, 0x1234       # 2 half-words
string:     .asciiz "MIPS Assembler"     # Null-terminated string
msg_start:  .asciiz "Memory Test: Starting...\n"
msg_word:   .asciiz "Memory Test: Word loaded = "
msg_byte:   .asciiz "Memory Test: Byte loaded = "
msg_pass:   .asciiz "Memory Test: PASSED\n"
newline:    .asciiz "\n"

.text
main:
    # Print start message
    li      $v0, 4              # Print string syscall
    la      $a0, msg_start      # Load address of start message
    syscall
    # Word operations
    li      $t0, 0x12345678     # Test value
    la      $t1, buffer         # Load address of buffer
    
    # Store word
    sw      $t0, 0($t1)         # Store at buffer[0]
    sw      $t0, 4($t1)         # Store at buffer[1]
    
    # Modify and store
    li      $t2, 0x87654321
    sw      $t2, 8($t1)         # Store at buffer[2]
    
    # Load words
    lw      $t3, 0($t1)         # Load from buffer[0]
    lw      $t4, 4($t1)         # Load from buffer[1]
    lw      $t5, 8($t1)         # Load from buffer[2]
    
    # Print a word result
    li      $v0, 4              # Print string syscall
    la      $a0, msg_word       # Load message
    syscall
    
    li      $v0, 1              # Print integer syscall
    move    $a0, $t3            # Print loaded word
    syscall
    
    li      $v0, 4              # Print string syscall
    la      $a0, newline        # Print newline
    syscall
    
    # Byte operations
    la      $t1, byte_data      # Load address of byte data
    
    # Load bytes
    lb      $t6, 0($t1)         # Load signed byte (0x12)
    lb      $t7, 1($t1)         # Load signed byte (0x34)
    lbu     $s0, 2($t1)         # Load unsigned byte (0x56)
    lbu     $s1, 3($t1)         # Load unsigned byte (0x78)
    
    # Print a byte result
    li      $v0, 4              # Print string syscall
    la      $a0, msg_byte       # Load message
    syscall
    
    li      $v0, 1              # Print integer syscall
    move    $a0, $t6            # Print loaded byte
    syscall
    
    li      $v0, 4              # Print string syscall
    la      $a0, newline        # Print newline
    syscall
    
    # Store bytes
    la      $t1, buffer         # Load address of buffer
    li      $t2, 0xAA
    sb      $t2, 12($t1)        # Store byte at buffer[3] (byte 0)
    li      $t2, 0xBB
    sb      $t2, 13($t1)        # Store byte at buffer[3] (byte 1)
    li      $t2, 0xCC
    sb      $t2, 14($t1)        # Store byte at buffer[3] (byte 2)
    li      $t2, 0xDD
    sb      $t2, 15($t1)        # Store byte at buffer[3] (byte 3)
    
    # Load word to verify byte stores
    lw      $s2, 12($t1)        # Should be 0xDDCCBBAA
    
    # Half-word operations
    la      $t1, half_data      # Load address of half-word data
    
    # Load half-words
    lh      $s3, 0($t1)         # Load signed half-word (0xABCD)
    lh      $s4, 2($t1)         # Load signed half-word (0x1234)
    lhu     $s5, 0($t1)         # Load unsigned half-word (0xABCD)
    
    # Store half-words
    la      $t1, buffer         # Load address of buffer
    li      $t2, 0xDEAD
    sh      $t2, 16($t1)        # Store half-word at buffer[4] (half-word 0)
    li      $t2, 0xBEEF
    sh      $t2, 18($t1)        # Store half-word at buffer[4] (half-word 1)
    
    # Load word to verify half-word stores
    lw      $s6, 16($t1)        # Should be 0xBEEFDEAD
    
    # Print success message
    li      $v0, 4              # Print string syscall
    la      $a0, msg_pass       # Load address of pass message
    syscall
    
    # Exit program
    li      $v0, 10
    syscall
