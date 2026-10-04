"""Pure decode/ALU functions. Intentional reference quirks are documented."""
from .types import *


def sign_extend(value: ac.u32, bits: ac.u32) -> ac.u32:
    sign = 1 << (bits - 1)
    return (value ^ sign) - sign


def next_rob(index: ac.u32) -> ac.u32:
    return index % 8 + 1


def decode(word: ac.u32) -> Instruction:
    op = word & 127
    ins = Instruction(word=word, opcode=op)
    if op == 0x33:
        ins.rd = (word >> 7) & 31
        ins.rs1 = (word >> 15) & 31
        ins.rs2 = (word >> 20) & 31
        ins.funct3 = (word >> 12) & 7
        ins.funct7 = word >> 25
    elif op == 0x13 or op == 3 or op == 0x67:
        ins.rd = (word >> 7) & 31
        ins.rs1 = (word >> 15) & 31
        ins.funct3 = (word >> 12) & 7
        ins.imm = sign_extend(word >> 20, 12)
    elif op == 0x23 or op == 0x63:
        ins.rs1 = (word >> 15) & 31
        ins.rs2 = (word >> 20) & 31
        ins.funct3 = (word >> 12) & 7
        if op == 0x23:
            ins.imm = sign_extend(((word >> 25) << 5) | ((word >> 7) & 31), 12)
        else:
            bits = ((word >> 31) << 12) | (((word >> 7) & 1) << 11)
            bits = bits | (((word >> 25) & 63) << 5) | (((word >> 8) & 15) << 1)
            ins.imm = sign_extend(bits, 13)
    elif op == 0x37 or op == 0x17:
        ins.rd = (word >> 7) & 31
        ins.imm = word & 0xfffff000
    elif op == 0x6f:
        ins.rd = (word >> 7) & 31
        bits = ((word >> 31) << 20) | (((word >> 12) & 255) << 12)
        bits = bits | (((word >> 20) & 1) << 11) | (((word >> 21) & 1023) << 1)
        ins.imm = sign_extend(bits, 21)
    else:
        ins = Instruction(word=0xffffffff, opcode=0x13)
    return ins


def alu_op(ins: Instruction) -> ac.u32:
    f = ins.funct3
    if ins.opcode == 0x63:
        return SUB if f < 2 else SLT if f < 6 else SLTU
    if ins.opcode != 0x13 and ins.opcode != 0x33:
        return ADD
    if f == 0:
        return SUB if ins.opcode == 0x33 and ins.funct7 == 32 else ADD
    if f == 1:
        return SLL
    if f == 2:
        return SLT
    if f == 3:
        return SLTU
    if f == 4:
        return XOR
    if f == 5:
        # Pinned skyzh tests immediate bit 9, not the ISA's bit 10.
        arithmetic = ins.funct7 == 32 if ins.opcode == 0x33 else (ins.imm & 512) != 0
        return SRA if arithmetic else SRL
    return OR if f == 6 else AND


def alu_value(op: ac.u32, a: ac.u32, b: ac.u32) -> ac.u32:
    if op == ADD:
        return a + b
    if op == SUB:
        return a - b
    if op == SLT:
        return ac.u32(ac.i32(a) < ac.i32(b))
    if op == SLTU:
        return ac.u32(a < b)
    if op == XOR:
        return a ^ b
    if op == OR:
        return a | b
    if op == AND:
        return a & b
    if op == SLL:
        return a << (b & 31)
    if op == SRL:
        return a >> (b & 31)
    return ac.u32(ac.i32(a) >> ac.i32(b & 31))


def branch_taken(function: ac.u32, value: ac.u32) -> bool:
    if function == 0 or function == 5 or function == 7:
        return value == 0
    if function == 1:
        return value != 0
    return value == 1
