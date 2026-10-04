"""Fixed immutable records: ordinary value structs in generated C++."""

from typing import NamedTuple


INVALID, ADD, ADDI, SUB, AND, OR, XOR, SLT, LUI, LW, SW, BEQ, BNE, JAL, JALR, HALT = range(16)


class Instruction(NamedTuple):
    word: int = 0
    op: int = INVALID
    rd: int = 0
    rs1: int = 0
    rs2: int = 0
    immediate: int = 0


def u32(value):
    return value & 0xffffffff


def signed(value):
    return value - (1 << 32) if value & (1 << 31) else value
