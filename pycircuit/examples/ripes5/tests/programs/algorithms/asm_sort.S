.text
.globl main
main:

la s0,values
li s1,5
outer:
li t0,0
addi t1,s1,-1
inner:
slli t2,t0,2
add t2,s0,t2
lw t3,0(t2)
lw t4,4(t2)
ble t3,t4,ordered
sw t4,0(t2)
sw t3,4(t2)
ordered:
addi t0,t0,1
blt t0,t1,inner
addi s1,s1,-1
bgt s1,zero,outer
li a0,0
li t0,1
checksum:
lw t1,0(s0)
mul t1,t1,t0
add a0,a0,t1
addi s0,s0,4
addi t0,t0,1
li t2,6
blt t0,t2,checksum
j done
.data
values: .word 5,1,4,2,3
.text
done:


li a7,93
ecall
