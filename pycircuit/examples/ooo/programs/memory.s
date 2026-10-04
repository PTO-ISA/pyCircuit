addi x1, x0, 73
sw x1, 0(x31)
lw x2, 0(x31)
addi x2, x2, 1
sw x2, 0(x31)
lw x3, 0(x31)
lw x4, 4(x31)
add x5, x3, x4
sw x5, 8(x31)
lw x6, 8(x31)
addi x7, x31, 12
sw x6, 0(x7)
lw x8, 12(x31)
halt
