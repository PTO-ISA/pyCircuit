.text
.globl main
main:
sh t0,0(a0)
ecall
li a0,42
li a7,93
ecall
