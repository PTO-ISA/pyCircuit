.option norvc
.option norelax
.text
.globl _start
_start:
    lui x20, 1
    lw x1, 0(x20)
    add x21, x1, x20
    lw x2, 0(x21)
    add x22, x2, x20
    lw x3, 0(x22)
    add x23, x3, x20
    lw x4, 0(x23)
    addi x5, x0, 1
    addi x6, x0, 2
    addi x7, x0, 3
    addi x8, x0, 4
    addi x9, x0, 5
    addi x10, x0, 6
    addi x11, x0, 7
    addi x12, x0, 8
    lui x28, 0x30
    addi x27, x0, 1
    sb x27, 4(x28)
