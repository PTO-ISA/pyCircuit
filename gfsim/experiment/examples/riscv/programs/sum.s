# Store 1..10, then load and sum them. All data is produced by instructions.
    addi x1, x0, 0
    addi x2, x0, 1
    addi x3, x0, 11
fill:
    sw x2, 0(x1)
    addi x1, x1, 4
    addi x2, x2, 1
    bne x2, x3, fill
    addi x1, x0, 0
    addi x2, x0, 10
    addi x4, x0, 0
sum:
    lw x5, 0(x1)
    add x4, x4, x5
    addi x1, x1, 4
    addi x2, x2, -1
    bne x2, x0, sum
    sw x4, 40(x0)
    halt
