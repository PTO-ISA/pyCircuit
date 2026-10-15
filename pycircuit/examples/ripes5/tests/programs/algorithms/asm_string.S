.text
.globl main
main:

la t0,message
li t1,0x1000
li a0,0
copy:
lbu t2,0(t0)
sb t2,0(t1)
lbu t3,0(t1)
bne t2,t3,fail
add a0,a0,t3
addi t0,t0,1
addi t1,t1,1
bne t2,zero,copy
j done
fail: li a0,0
j done
message: .asciz "Hello"
.balign 4
done:


li a7,93
ecall
