.text
.globl main
main:

li a0,6
call factorial
j done
factorial:
addi sp,sp,-16
sw ra,12(sp)
sw a0,8(sp)
li t0,2
blt a0,t0,base
addi a0,a0,-1
call factorial
lw t0,8(sp)
mul a0,a0,t0
j restore
base: li a0,1
restore: lw ra,12(sp)
addi sp,sp,16
ret
done:


li a7,93
ecall
