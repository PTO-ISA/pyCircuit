.text
.globl main
main:

lui x10,1
addi x1,x0,41
nop
nop
addi x2,x1,1
sw x2,0(x10)
lw x3,0(x10)
nop
add x4,x3,x1
sw x4,4(x10)

addi zero,zero,2047
1: j 1b
nop
nop
