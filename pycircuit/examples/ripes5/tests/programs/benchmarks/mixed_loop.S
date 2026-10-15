.text
.globl main
main:
li s0,0x1000
li t0,1
li t1,7
li t2,13
loop:
add t0,t0,t1
xor t2,t2,t0
andi t2,t2,1023
slli t3,t2,3
mul t4,t0,t2
mulh t5,t4,t0
mulhsu t6,t4,t2
divu s1,t4,t1
remu s2,t0,t1
sw t4,0(s0)
lw s3,0(s0)
add s4,s3,t0
beq s3,t4,taken
sw zero,0(s0)
addi s5,s5,1
taken:
xor t0,t0,s2
j loop
