.text
.globl main
main:
li s0,0x1000
c.ebreak
sw zero,0(s0)
li a0,42
li a7,93
ecall
