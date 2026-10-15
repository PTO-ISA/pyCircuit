.text
.globl main
main:

    addi x1,x0,3
loop:
    addi x1,x1,-1
    bne x1,x0,loop
    beq x1,x0,call
    addi x20,x0,99
call:
    jal x5,subroutine
    addi x6,x5,1
    jal x0,done
subroutine:
    addi x7,x5,2
    jalr x8,0(x5)
    addi x21,x0,99
done:
    add x9,x7,x8

addi zero,zero,2047
1: j 1b
nop
nop
