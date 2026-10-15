.text
.globl main
main:
li a0,42
li a7,93
ecall
.word 0x13,0x13,0x13
.org 4090
.option norvc
jal zero,main
.2byte 0x0003
