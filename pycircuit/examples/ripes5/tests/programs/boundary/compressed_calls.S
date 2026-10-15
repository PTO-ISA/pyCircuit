.text
.globl main
main:
c.nop
.option push
.option norvc
addi t0,zero,7
.option pop
c.jal called
li a0,42
j done
called:
c.addi t0,1
c.jr ra
done:


li a7,93
ecall
