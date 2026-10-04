addi x20, x0, 4
loop:
lw x2, 12(x31)
lw x15, 52(x31)
beq x10, x9, skip_2
addi x13, x13, 1
skip_2:
xor x12, x13, x13
lw x8, 8(x31)
addi x2, x5, 25
sw x14, 60(x31)
and x11, x6, x4
beq x15, x6, skip_8
addi x6, x6, 1
skip_8:
sw x12, 8(x31)
lw x12, 36(x31)
sw x8, 36(x31)
and x1, x14, x12
xor x8, x7, x2
addi x9, x8, 22
beq x13, x13, skip_15
addi x9, x9, 1
skip_15:
xor x10, x15, x8
lw x5, 60(x31)
addi x9, x9, -4
addi x13, x3, -19
sub x12, x10, x7
beq x15, x4, skip_21
addi x8, x8, 1
skip_21:
sw x2, 56(x31)
lw x11, 32(x31)
xor x9, x7, x11
sw x11, 60(x31)
sub x13, x15, x7
addi x5, x14, -28
lw x3, 56(x31)
and x13, x9, x15
sw x4, 60(x31)
beq x9, x13, skip_31
addi x8, x8, 1
skip_31:
lw x8, 4(x31)
add x14, x5, x6
beq x12, x12, skip_34
addi x8, x8, 1
skip_34:
addi x9, x5, 22
addi x14, x12, 2
addi x15, x8, -8
addi x12, x14, 25
addi x15, x1, 25
beq x9, x15, skip_40
addi x10, x10, 1
skip_40:
addi x2, x10, -1
beq x10, x1, skip_42
addi x2, x2, 1
skip_42:
beq x5, x11, skip_43
addi x9, x9, 1
skip_43:
addi x11, x6, 8
beq x10, x10, skip_45
addi x11, x11, 1
skip_45:
sw x14, 16(x31)
lw x9, 16(x31)
addi x20, x20, -1
bne x20, x0, loop
halt
