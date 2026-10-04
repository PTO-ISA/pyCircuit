addi x20, x0, 12
loop:
lw x1, 0(x31)
add x2, x1, x1
add x3, x2, x2
bne x3, x0, target
lw x8, 2(x31)
sw x2, 4(x31)
sw x2, -4(x0)
halt
target:
addi x20, x20, -1
addi x9, x9, 1
addi x10, x10, 1
addi x11, x11, 1
addi x12, x12, 1
addi x13, x13, 1
addi x14, x14, 1
addi x15, x15, 1
bne x20, x0, loop
halt
