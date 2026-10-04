lui x1, 0x80000
addi x2, x0, -1
slt x3, x1, x2
slt x4, x2, x1
add x5, x1, x1
sub x6, x5, x2
and x7, x1, x2
or x8, x1, x6
xor x9, x2, x1
jal x10, subroutine
beq x0, x0, done
subroutine:
addi x11, x10, 1
jalr x12, 0(x11)
done:
halt
