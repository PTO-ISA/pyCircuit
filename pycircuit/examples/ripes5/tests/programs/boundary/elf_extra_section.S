.text
.globl main
main:
la t0,datum
lw a0,0(t0)
li a7,93
ecall
.section .extra,"aw",@progbits
datum: .word 42
