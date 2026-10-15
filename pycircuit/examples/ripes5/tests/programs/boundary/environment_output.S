.text
.globl main
main:
li a0,-123
li a7,1
ecall
li a0,32
li a7,11
ecall
la a0,message
li a7,4
ecall
li a0,42
j done
.section .rodata
message: .asciz "RV32IMC!"
.text
done:


li a7,93
ecall
