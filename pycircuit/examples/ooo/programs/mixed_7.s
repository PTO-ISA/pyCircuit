addi x20, x0, 4
loop:
sw x3, 4(x31)
addi x2, x14, 14
lw x10, 24(x31)
add x1, x2, x7
add x4, x2, x9
addi x14, x10, -25
addi x10, x10, -4
addi x1, x9, 5
addi x7, x3, 7
addi x9, x14, -19
addi x10, x10, 15
addi x2, x9, -25
sw x4, 52(x31)
lw x13, 56(x31)
beq x5, x4, skip_14
addi x6, x6, 1
skip_14:
addi x3, x12, -22
and x10, x5, x9
lw x12, 8(x31)
addi x2, x9, 11
add x3, x15, x8
lw x11, 40(x31)
lw x6, 60(x31)
addi x10, x13, -21
sw x8, 8(x31)
slt x1, x12, x12
and x10, x11, x14
sw x7, 44(x31)
addi x1, x8, -18
beq x1, x4, skip_28
addi x8, x8, 1
skip_28:
addi x5, x3, 18
add x7, x15, x14
lw x3, 32(x31)
xor x15, x3, x14
slt x5, x12, x7
addi x15, x7, -22
sw x3, 28(x31)
lw x1, 20(x31)
addi x5, x5, 21
lw x9, 40(x31)
lw x3, 4(x31)
beq x15, x14, skip_40
addi x8, x8, 1
skip_40:
lw x14, 48(x31)
addi x7, x7, 29
addi x11, x7, -24
addi x4, x8, 11
addi x10, x1, -13
lw x9, 0(x31)
lw x2, 48(x31)
addi x20, x20, -1
bne x20, x0, loop
halt
