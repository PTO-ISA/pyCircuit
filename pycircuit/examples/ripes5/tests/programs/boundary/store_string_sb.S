.text
.globl main
main:
sb t0,0(a0)
ecall
li a0,42
li a7,93
ecall
