.text
.globl main
main:
li a0,42
li a7,93
ecall
.org 4092
jal zero,main
