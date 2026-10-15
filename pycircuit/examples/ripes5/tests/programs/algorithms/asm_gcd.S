.text
.globl main
main:

li a0,1071
li a1,462
loop:
beq a1,zero,done
remu t0,a0,a1
mv a0,a1
mv a1,t0
j loop
done:


li a7,93
ecall
