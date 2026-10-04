.option norvc
.option norelax
.text
.globl _start
_start:
    lui x1, 1
    lui x2, 0x80ff8
    addi x2, x2, 0x234
    sw x2, 0(x1)
    lw x3, 0(x1)
    lb x4, 2(x1)
    lbu x5, 2(x1)
    lh x6, 2(x1)
    lhu x7, 2(x1)
    addi x8, x0, -128
    sb x8, 1(x1)
    addi x9, x0, -2
    sh x9, 2(x1)
    lw x10, 0(x1)
    lui x28, 0x30
    addi x27, x0, 1
    sb x27, 4(x28)
