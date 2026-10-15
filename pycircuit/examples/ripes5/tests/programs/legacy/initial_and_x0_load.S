.text
.globl main
main:

add x3,x1,x2
lw x0,0(x10)
addi x4,x0,4
lw x5,0(x10)
sw x5,4(x10)
lw x6,4(x10)
add x7,x6,x3

addi zero,zero,2047
1: j 1b
nop
nop
