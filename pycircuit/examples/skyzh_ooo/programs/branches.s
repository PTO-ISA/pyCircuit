.option norvc
.option norelax
.text
.globl _start
_start:
    addi x2, x0, 32
    addi x3, x0, 0
    addi x10, x0, 0
loop:
    add x10, x10, x3
    addi x3, x3, 1
    bne x3, x2, loop
    jal x1, function
    beq x3, x2, equal
    sw x3, 0(x0)       # Wrong-path Store must not commit.
equal:
    blt x0, x3, less
    addi x10, x0, -1
less:
    bge x3, x2, greater
    addi x10, x0, -2
greater:
    bltu x0, x3, unsigned_less
    addi x10, x0, -3
unsigned_less:
    bgeu x3, x2, finish
    addi x10, x0, -4
finish:
    lui x28, 0x30
    addi x27, x0, 1
    sb x27, 4(x28)
    jal x0, finish
function:
    addi x10, x10, 7
    jalr x0, x1, 0
