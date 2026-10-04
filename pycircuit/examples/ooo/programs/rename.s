lw x1, 0(x31)
add x2, x1, x1
addi x1, x0, 99
add x3, x1, x2
addi x1, x1, 1
addi x1, x1, 1
addi x0, x1, 9
lw x0, 4(x31)
add x4, x0, x1
addi x5, x0, 18
loop:
addi x6, x6, 1
add x7, x6, x1
addi x1, x1, -1
addi x5, x5, -1
bne x5, x0, loop
halt
