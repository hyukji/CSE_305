        .data
var:  .word   5
var1:  .word   10
array2: .word   0x12345678
        .word   0xFFFFFFFF
var12:  .word   620
        .text
main:
    la $8, var
    la $10, var
    lw $9, 0($8)
        sll     $6, $1, 4
        or      $4, $3, $2
    addu $2, $0, $9
        nor     $9, $4, $3
    jal sum
    j exit

sum: sltiu $1, $2, 1
    bne $1, $0, sum_exit
    addu $3, $3, $2
    beq $3, $2 main
    sltu $4, $3, $2
    addiu $2, $2, -1
    lui $17, 100
        and     $13, $11, $5
        andi    $14, $4, -3
    j sum
sum_exit:
    addu $4, $3, $0
    jr $31
exit:
