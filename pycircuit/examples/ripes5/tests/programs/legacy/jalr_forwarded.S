.text
.globl main
main:

lui x10,1
addi x1,x0,24
jalr x5,0(x1)
sw x0,0(x10)
addi x20,x0,99
nop
addi x6,x5,1
lw x2,0(x10)
jalr x7,0(x2)
sw x0,0(x10)
add x8,x5,x7

addi zero,zero,2047
1: j 1b
nop
nop
