.text
.globl main
main:
li s0,0x1000
li t0,7
sw t0,0(s0)
lw t1,0(s0)
beq t0,t1,taken
.word 0xffffffff
sw zero,0(s0)
taken: li a0,42

li a7,93
ecall
