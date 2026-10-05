"""Decoder for the Ripes5 instruction subset."""

from .records import (Instruction, INVALID, ADD, ADDI, SUB, AND, OR, XOR,
                      SLT, LUI, LW, SW, BEQ, BNE, JAL, JALR, HALT)


def sext(value, width):
    sign = 1 << (width - 1)
    return (value ^ sign) - sign


def decode(word):
    opcode, f3, f7 = word & 127, (word >> 12) & 7, word >> 25
    rd, rs1, rs2 = (word >> 7) & 31, (word >> 15) & 31, (word >> 20) & 31
    op, immediate = INVALID, 0
    if opcode == 0x33:
        op = {(0, 0): ADD, (0, 32): SUB, (7, 0): AND,
              (6, 0): OR, (4, 0): XOR, (2, 0): SLT}.get((f3, f7), INVALID)
    elif opcode in (0x13, 0x03, 0x67):
        op = {(0x13, 0): ADDI, (0x03, 2): LW, (0x67, 0): JALR}.get((opcode, f3), INVALID)
        immediate, rs2 = sext(word >> 20, 12), 0
    elif opcode == 0x37:
        op, immediate, rs1, rs2 = LUI, word & 0xfffff000, 0, 0
    elif opcode == 0x23 and f3 == 2:
        op, immediate, rd = SW, sext(((word >> 25) << 5) | ((word >> 7) & 31), 12), 0
    elif opcode == 0x63 and f3 in (0, 1):
        bits = ((word >> 31) << 12) | (((word >> 7) & 1) << 11)
        bits |= (((word >> 25) & 63) << 5) | (((word >> 8) & 15) << 1)
        op, immediate, rd = (BEQ if f3 == 0 else BNE), sext(bits, 13), 0
    elif opcode == 0x6f:
        bits = ((word >> 31) << 20) | (((word >> 12) & 255) << 12)
        bits |= (((word >> 20) & 1) << 11) | (((word >> 21) & 1023) << 1)
        op, immediate, rs1, rs2 = JAL, sext(bits, 21), 0, 0
    elif word == 0x00100073:
        op, rd, rs1, rs2 = HALT, 0, 0, 0
    return Instruction(word, op, rd, rs1, rs2, immediate)


def sources(instruction):
    op = instruction.op
    first = op in (ADD, ADDI, SUB, AND, OR, XOR, SLT, LW, SW, BEQ, BNE, JALR)
    second = op in (ADD, SUB, AND, OR, XOR, SLT, SW, BEQ, BNE)
    return instruction.rs1 if first else 0, instruction.rs2 if second else 0
