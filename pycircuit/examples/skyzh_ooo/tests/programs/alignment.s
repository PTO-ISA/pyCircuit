.option norvc
.option norelax
.text
.globl _start
_start:
    lui x20, 1
    addi x1, x0, 1
    addi x2, x0, 2
    addi x1, x0, 3      # Rename x1 in the cycle its older writer commits.
    add x3, x1, x2
    sw x3, 0(x20)
    lw x4, 0(x20)
    add x5, x4, x1
    add x6, x4, x2
    xor x7, x4, x3     # Several stations wake and complete together.
    add x8, x4, x4
    add x9, x5, x6
    sw x9, 4(x20)
    lw x10, 4(x20)
    lw x11, 0(x20)
    sw x11, 8(x20)
    addi x12, x0, 12
loop:
    addi x12, x12, -1
    add x13, x13, x10
    bne x12, x0, loop
    # Guaranteed mispredict while younger memory/ALU messages are in flight.
    beq x0, x0, target
    lw x14, 0(x20)
    sw x0, 0(x20)
    addi x13, x0, -1
target:
    jal x1, function
    lui x28, 0x30
    addi x27, x0, 1
    sb x27, 4(x28)
    jal x0, target
function:
    addi x13, x13, 7
    jalr x0, x1, 0
