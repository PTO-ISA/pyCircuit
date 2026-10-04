addi x20, x0, 16
loop:
addi x20, x20, -1
bne x20, x0, loop
lw x1, 0(x31)
lw x2, 4(x31)
lw x3, 8(x31)
lw x4, 12(x31)
halt
