.text
.globl main
main:

    lui x10,1
    addi x1,x0,1
    addi x2,x0,9
fill:
    sw x1,0(x10)
    addi x10,x10,4
    addi x1,x1,1
    bne x1,x2,fill
    lui x10,1
    addi x1,x0,8
    addi x3,x0,0
sum:
    lw x4,0(x10)
    add x3,x3,x4
    addi x10,x10,4
    addi x1,x1,-1
    bne x1,x0,sum
    sw x3,0(x10)

addi zero,zero,2047
1: j 1b
nop
nop
