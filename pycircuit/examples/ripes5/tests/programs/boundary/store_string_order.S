.text
.globl main
main:
sw t0,0(a0)
ecall
ecall
li a7,11
li a0,33
ecall
li a7,1
li a0,-7
ecall
li a0,42
li a7,93
ecall
