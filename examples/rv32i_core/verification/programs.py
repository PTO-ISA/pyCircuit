"""Finite test programs; semantic expectations come from the separate Spike run."""

from __future__ import annotations
import random

SMOKE = """addi x1,x0,7
addi x2,x1,5
add x3,x2,x1
lui x4,0x80008
sw x3,0(x4)
lw x5,0(x4)
add x6,x5,x3
"""

DIRECTED = """lui x20,0x80008
addi x1,x0,-17
addi x2,x0,5
add x3,x1,x2
sub x4,x2,x1
sll x5,x1,x2
slt x6,x1,x2
sltu x7,x1,x2
xor x8,x1,x2
srl x9,x1,x2
sra x10,x1,x2
or x11,x1,x2
and x12,x1,x2
addi x13,x1,31
slti x14,x1,-16
sltiu x15,x1,-16
xori x16,x1,-1
ori x17,x1,3
andi x18,x1,127
slli x19,x1,31
srli x21,x1,31
srai x22,x1,31
lui x23,0xabcde
auipc x24,0
addi x0,x1,999
sw x1,0(x20)
sb x2,1(x20)
sh x1,2(x20)
lb x25,0(x20)
lbu x26,0(x20)
lh x27,2(x20)
lhu x28,2(x20)
lw x29,0(x20)
add x30,x29,x2
sw x30,4(x20)
lw x0,4(x20)
fence iorw,iorw
.word 0xfff0808f
beq x1,x1,1f
sw x0,4(x20)
1: bne x1,x2,2f
sw x0,4(x20)
2: blt x1,x2,3f
sw x0,4(x20)
3: bge x2,x1,4f
sw x0,4(x20)
4: bltu x2,x1,5f
sw x0,4(x20)
5: bgeu x1,x2,6f
sw x0,4(x20)
6: beq x1,x2,7f
bne x1,x1,7f
blt x2,x1,7f
bge x1,x2,7f
bltu x1,x2,7f
bgeu x2,x1,7f
jal x31,8f
7: sw x0,4(x20)
addi x30,x0,-1
8: auipc x9,0
addi x9,x9,17
jalr x8,0(x9)
sw x0,4(x20)
addi x10,x8,0
"""

TRAPS = {
    "illegal32": ".word 0xffffffff",
    "illegal_m": ".word 0x022081b3",
    "illegal16": ".word 0x00000000",
    "illegal16_upper": ".word 0xabcd0000",
    "ecall": "ecall",
    "breakpoint": "ebreak",
    "load_half_alignment": "lui x1,0x80008\nlh x2,1(x1)",
    "load_word_alignment": "lui x1,0x80008\nlw x2,2(x1)",
    "store_half_alignment": "lui x1,0x80008\nsh x2,1(x1)",
    "store_word_alignment": "lui x1,0x80008\nsw x2,2(x1)",
    "jump_alignment": "jal x1,2",
    "jalr_alignment": "lui x1,0x80000\naddi x1,x1,2\njalr x2,0(x1)",
    "branch_alignment": "beq x0,x0,2",
    "instruction_access": "lui x1,0x90000\njalr x0,0(x1)",
    "load_access": "lui x1,0x90000\nlw x2,0(x1)",
    "load_x0_access": "lui x1,0x90000\nlw x0,0(x1)",
    "store_access": "lui x1,0x90000\nsw x2,0(x1)",
    "illegal_csr": ".word 0xc00020f3",
    "illegal_fence_i": ".word 0x0000100f",
}


def random_program(seed, length=96):
    rng = random.Random(seed)
    lines = ["lui x20,0x80008"] + [
        f"addi x{r},x0,{rng.randrange(-2048,2048)}" for r in range(1, 20)
    ]
    for n in range(length):
        rd, a, b = [rng.randrange(0, 20) for _ in range(3)]
        family = rng.randrange(5)
        if family == 0:
            lines.append(
                f'{rng.choice(["add","sub","sll","slt","sltu","xor","srl","sra","or","and"])} x{rd},x{a},x{b}'
            )
        elif family == 1:
            lines.append(
                f'{rng.choice(["addi","slti","sltiu","xori","ori","andi"])} x{rd},x{a},{rng.randrange(-2048,2048)}'
            )
        elif family == 2:
            lines.append(
                f'{rng.choice(["slli","srli","srai"])} x{rd},x{a},{rng.randrange(32)}'
            )
        elif family == 3:
            width = rng.choice([1, 2, 4])
            offset = rng.randrange(0, 128) // width * width
            lines += [
                f'{ {1:"sb",2:"sh",4:"sw"}[width]} x{a},{offset}(x20)',
                f'{ {1:"lbu",2:"lh",4:"lw"}[width]} x{rd},{offset}(x20)',
            ]
        else:
            lines += [
                f'{rng.choice(["beq","bne","blt","bge","bltu","bgeu"])} x{a},x{b},L{n}',
                f"addi x{rd},x{rd},1",
                f"L{n}: addi x0,x0,0",
            ]
    return "\n".join(lines) + "\n"


def assembly_cases():
    cases = {
        "smoke": SMOKE,
        "families": DIRECTED,
        "throughput": "\n".join(f"addi x{1+n%30},x0,{n}" for n in range(64)),
        "untaken_misaligned_branch": "addi x1,x0,1\nbeq x0,x1,2\naddi x2,x0,7",
    }
    for seed in [1, 17, 20261102]:
        cases[f"random-{seed}"] = random_program(seed)
    cases.update(TRAPS)
    return {
        name: ".section .text.init\n.globl _start\n_start:\n"
        + text
        + "\nebreak\nsw x0,0(x0)\n"
        for name, text in cases.items()
    }


C_DEMO = """volatile unsigned signature[4];
unsigned values[32];
void main(void) {
 unsigned sum=0, mix=0x12345678u;
 for(unsigned i=0;i<32;i++) values[i]=(i<<2)^0x135u;
 for(unsigned i=0;i<32;i++){sum+=values[i];mix=(mix<<5)^(mix>>27)^values[i];}
 signature[0]=sum;signature[1]=mix;signature[2]=values[0];signature[3]=values[31];
 __asm__ volatile("ebreak");
 for(;;){}
}
"""
