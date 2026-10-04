.option norvc
.option norelax
.text
.globl _start
_start:
    addi x1, x0, 7
    auipc x10, 1
    lui x28, 0x30
    addi x27, x0, 1
    sb x27, 4(x28)
