.text
.globl main
main:
la t0,target
ori t0,t0,1
jalr zero,0(t0)
sw zero,0(s0)
.word 0xffffffff
target: li a0,42
li a7,93
ecall
