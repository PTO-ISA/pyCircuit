.text
.globl main
main:

addi x1,x0,7
addi x2,x0,-3
lui x3,0x80000
add x4,x1,x2
sub x5,x1,x2
and x6,x1,x2
or x7,x1,x2
xor x8,x1,x2
slt x9,x2,x1
add x10,x3,x3

addi zero,zero,2047
1: j 1b
nop
nop
