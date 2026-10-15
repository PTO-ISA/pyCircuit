.text
.globl main
main:

    lui x10,1
    addi x1,x0,17
    sw x1,0(x10)
    lw x2,0(x10)
    beq x2,x1,taken
    sw x0,0(x10)
    addi x20,x0,99
taken:
    addi x3,x2,1
    bne x3,x2,next
    sw x0,0(x10)
    addi x21,x0,99
next:
    lw x4,0(x10)
    bne x4,x1,bad
    addi x5,x4,1
    jal x0,done
bad:
    sw x0,0(x10)
done:
    sw x5,4(x10)

addi zero,zero,2047
1: j 1b
nop
nop
