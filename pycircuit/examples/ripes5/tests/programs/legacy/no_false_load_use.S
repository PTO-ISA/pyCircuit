.text
.globl main
main:

lw x5,0(x10)
addi x6,x0,5
lw x5,0(x10)
lui x7,0x528
lw x5,0(x10)
addi x0,x0,5

addi zero,zero,2047
1: j 1b
nop
nop
