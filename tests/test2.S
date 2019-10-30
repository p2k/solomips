
    .data
array1:
    .space 12
array2:
    .space 12

    .text
    .globl main
main:
    la $t0, array1
	li $t1, 5
	sw $t1, ($t0)
	li $t1, 13
    la $t0, array2
	sw $t1, 4($t0)
	li $t1, -7
	sw $t1, 8($t0)
    jr $zero
