
    .text
    .set noreorder
    .globl main
main:
    li $v0, 65532
    j test2
    nop

test1:
    jr $zero
    nop

test2:
    j test1
    nop
