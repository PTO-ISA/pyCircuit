.text
.globl main
main:

addi x1,x0,1
addi x1,x1,2
addi x1,x1,4
add x2,x1,x1
addi x0,x2,99
add x3,x0,x2
add x4,x1,x3

addi zero,zero,2047
1: j 1b
nop
nop
