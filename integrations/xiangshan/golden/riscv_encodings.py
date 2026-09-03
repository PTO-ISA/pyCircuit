"""RISC-V instruction encoders for generating test programs.

Used by L4 end-to-end tests to create instruction sequences that can
be injected into IBuffer and traced through the pipeline.
"""

from __future__ import annotations

from golden.decode_vectors import (
    encode_b,
    encode_i,
    encode_j,
    encode_r,
    encode_s,
    encode_u,
)


# ── Convenience wrappers ─────────────────────────────────────────


def nop() -> int:
    """ADDI x0, x0, 0"""
    return encode_i(0x13, rd=0, funct3=0, rs1=0, imm12=0)


def addi(rd: int, rs1: int, imm: int) -> int:
    return encode_i(0x13, rd=rd, funct3=0, rs1=rs1, imm12=imm & 0xFFF)


def add(rd: int, rs1: int, rs2: int) -> int:
    return encode_r(0x33, rd=rd, funct3=0, rs1=rs1, rs2=rs2, funct7=0)


def sub(rd: int, rs1: int, rs2: int) -> int:
    return encode_r(0x33, rd=rd, funct3=0, rs1=rs1, rs2=rs2, funct7=0x20)


def lw(rd: int, rs1: int, offset: int) -> int:
    return encode_i(0x03, rd=rd, funct3=2, rs1=rs1, imm12=offset & 0xFFF)


def sw(rs2: int, rs1: int, offset: int) -> int:
    return encode_s(0x23, funct3=2, rs1=rs1, rs2=rs2, imm12=offset & 0xFFF)


def beq(rs1: int, rs2: int, offset: int) -> int:
    return encode_b(0x63, funct3=0, rs1=rs1, rs2=rs2, imm13=offset & 0x1FFF)


def bne(rs1: int, rs2: int, offset: int) -> int:
    return encode_b(0x63, funct3=1, rs1=rs1, rs2=rs2, imm13=offset & 0x1FFF)


def jal(rd: int, offset: int) -> int:
    return encode_j(0x6F, rd=rd, imm21=offset & 0x1FFFFF)


def lui(rd: int, imm20: int) -> int:
    return encode_u(0x37, rd=rd, imm20=imm20)


# ── Pre-built test programs ──────────────────────────────────────

PROGRAM_SINGLE_ADD = [
    addi(1, 0, 5),  # x1 = 5
    addi(2, 0, 3),  # x2 = 3
    add(3, 1, 2),  # x3 = x1 + x2 = 8
]

PROGRAM_RAW_CHAIN = [
    addi(1, 0, 10),  # x1 = 10
    addi(2, 1, 20),  # x2 = x1 + 20 = 30 (RAW on x1)
    add(3, 1, 2),  # x3 = x1 + x2 = 40 (RAW on x1 and x2)
]

PROGRAM_LOAD_STORE = [
    addi(1, 0, 42),  # x1 = 42
    sw(1, 0, 0),  # mem[0] = x1
    lw(2, 0, 0),  # x2 = mem[0] = 42
]

PROGRAM_BRANCH = [
    addi(1, 0, 5),  # x1 = 5
    addi(2, 0, 5),  # x2 = 5
    beq(1, 2, 8),  # if x1==x2, skip next
    addi(3, 0, 99),  # x3 = 99 (skipped)
    addi(4, 0, 42),  # x4 = 42 (branch target)
]
