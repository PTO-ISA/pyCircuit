.text
.globl main
main:

la s0,left
la s1,right
li a0,0
li t0,0
row:
li t1,0
column:
li t2,0
li t3,0
inner:
slli t4,t0,1
add t4,t4,t2
slli t4,t4,2
add t4,s0,t4
lw t5,0(t4)
slli t4,t2,1
add t4,t4,t1
slli t4,t4,2
add t4,s1,t4
lw t6,0(t4)
mul t5,t5,t6
add t3,t3,t5
addi t2,t2,1
li t4,2
blt t2,t4,inner
add a0,a0,t3
addi t1,t1,1
blt t1,t4,column
addi t0,t0,1
blt t0,t4,row
j done
.data
left: .word 1,2,3,4
right: .word 5,6,7,8
.text
done:


li a7,93
ecall
