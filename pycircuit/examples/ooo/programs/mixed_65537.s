addi x20, x0, 4
loop:
addi x14, x7, 1
addi x3, x15, 11
xor x4, x3, x1
addi x11, x3, 4
slt x14, x9, x4
and x11, x12, x10
and x11, x3, x2
slt x1, x2, x15
addi x8, x12, -15
add x8, x1, x4
beq x14, x13, skip_10
addi x5, x5, 1
skip_10:
beq x1, x5, skip_11
addi x14, x14, 1
skip_11:
sw x4, 8(x31)
xor x15, x3, x3
sw x7, 44(x31)
lw x15, 28(x31)
addi x7, x7, 3
addi x5, x5, 13
addi x8, x1, -17
sw x11, 16(x31)
addi x8, x3, -17
addi x14, x8, -4
sub x4, x9, x9
sw x11, 24(x31)
lw x1, 48(x31)
and x7, x8, x15
and x12, x2, x11
beq x12, x12, skip_27
addi x8, x8, 1
skip_27:
sw x11, 12(x31)
beq x15, x10, skip_29
addi x5, x5, 1
skip_29:
lw x11, 52(x31)
beq x1, x14, skip_31
addi x11, x11, 1
skip_31:
addi x7, x12, -23
addi x1, x2, -30
sw x6, 48(x31)
add x6, x14, x2
sw x9, 12(x31)
sw x15, 36(x31)
sub x9, x3, x7
addi x6, x8, -10
lw x12, 48(x31)
and x15, x3, x10
beq x10, x3, skip_42
addi x2, x2, 1
skip_42:
slt x6, x12, x11
addi x13, x9, 4
addi x12, x13, 23
xor x12, x8, x10
xor x9, x9, x15
addi x20, x20, -1
bne x20, x0, loop
halt
