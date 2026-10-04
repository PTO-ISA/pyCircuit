.option norvc
.option norelax
.text
.globl _start
_start:
    lui x1, 1
    addi x2, x0, 64
    addi x3, x0, 1
    sw x3, 0(x1)
loop:
    lw x4, 0(x1)
    addi x5, x5, 1
    addi x6, x6, 2
    addi x7, x7, 3
    add x8, x4, x3
    xor x9, x5, x6
    add x10, x8, x9
    sw x8, 0(x1)
    addi x2, x2, -1
    bne x2, x0, loop
    lui x28, 0x30
    addi x27, x0, 1
    sb x27, 4(x28)
