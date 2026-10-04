.option norvc
.option norelax
.text
.globl _start
_start:
    addi x1, x0, -16
    addi x2, x0, 3
    add x3, x1, x2
    sub x4, x2, x1
    sll x5, x1, x2
    slt x6, x1, x2
    sltu x7, x1, x2
    xor x8, x1, x2
    srl x9, x1, x2
    sra x10, x1, x2
    or x11, x1, x2
    and x12, x1, x2
    slli x13, x2, 4
    slti x14, x1, -1
    sltiu x15, x1, -1
    xori x16, x1, 255
    srli x17, x1, 3
    srai x18, x1, 3
    ori x19, x2, 256
    andi x20, x1, 255
    lui x21, 0x12345
    lui x28, 0x30
    addi x27, x0, 1
    sb x27, 4(x28)
