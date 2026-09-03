"""Golden decode test vectors — RISC-V instruction encoding → expected decode fields.

Each vector: (inst32, expected_rd, expected_rs1, expected_rs2,
              expected_imm, expected_is_branch, expected_is_load,
              expected_is_store, expected_use_imm, description)

Immediate values are unsigned (bit-extracted), matching RTL behaviour.
"""

from __future__ import annotations


def _bits(val: int, hi: int, lo: int) -> int:
    return (val >> lo) & ((1 << (hi - lo + 1)) - 1)


def _sign_ext(val: int, nbits: int, target: int = 32) -> int:
    sign = (val >> (nbits - 1)) & 1
    if sign:
        return val | (((1 << (target - nbits)) - 1) << nbits)
    return val


# ── RV64I instruction encoders ──────────────────────────────────


def encode_r(opcode: int, rd: int, funct3: int, rs1: int, rs2: int, funct7: int) -> int:
    return (
        (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode
    )


def encode_i(opcode: int, rd: int, funct3: int, rs1: int, imm12: int) -> int:
    return ((imm12 & 0xFFF) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode


def encode_s(opcode: int, funct3: int, rs1: int, rs2: int, imm12: int) -> int:
    imm_11_5 = (imm12 >> 5) & 0x7F
    imm_4_0 = imm12 & 0x1F
    return (
        (imm_11_5 << 25)
        | (rs2 << 20)
        | (rs1 << 15)
        | (funct3 << 12)
        | (imm_4_0 << 7)
        | opcode
    )


def encode_b(opcode: int, funct3: int, rs1: int, rs2: int, imm13: int) -> int:
    b12 = (imm13 >> 12) & 1
    b10_5 = (imm13 >> 5) & 0x3F
    b4_1 = (imm13 >> 1) & 0xF
    b11 = (imm13 >> 11) & 1
    return (
        (b12 << 31)
        | (b10_5 << 25)
        | (rs2 << 20)
        | (rs1 << 15)
        | (funct3 << 12)
        | (b4_1 << 8)
        | (b11 << 7)
        | opcode
    )


def encode_u(opcode: int, rd: int, imm20: int) -> int:
    return ((imm20 & 0xFFFFF) << 12) | (rd << 7) | opcode


def encode_j(opcode: int, rd: int, imm21: int) -> int:
    b20 = (imm21 >> 20) & 1
    b10_1 = (imm21 >> 1) & 0x3FF
    b11 = (imm21 >> 11) & 1
    b19_12 = (imm21 >> 12) & 0xFF
    return (
        (b20 << 31) | (b10_1 << 21) | (b11 << 20) | (b19_12 << 12) | (rd << 7) | opcode
    )


# ── Standard instructions ───────────────────────────────────────

# R-type: ADD x1, x2, x3
INST_ADD_X1_X2_X3 = encode_r(0x33, rd=1, funct3=0, rs1=2, rs2=3, funct7=0)

# R-type: SUB x4, x5, x6
INST_SUB_X4_X5_X6 = encode_r(0x33, rd=4, funct3=0, rs1=5, rs2=6, funct7=0x20)

# I-type: ADDI x7, x8, 42
INST_ADDI_X7_X8_42 = encode_i(0x13, rd=7, funct3=0, rs1=8, imm12=42)

# I-type: LW x5, 8(x10)
INST_LW_X5_8_X10 = encode_i(0x03, rd=5, funct3=2, rs1=10, imm12=8)

# S-type: SW x3, 16(x4)
INST_SW_X3_16_X4 = encode_s(0x23, funct3=2, rs1=4, rs2=3, imm12=16)

# B-type: BEQ x1, x2, +8
INST_BEQ_X1_X2_8 = encode_b(0x63, funct3=0, rs1=1, rs2=2, imm13=8)

# U-type: LUI x10, 0xDEAD0
INST_LUI_X10 = encode_u(0x37, rd=10, imm20=0xDEAD0)

# J-type: JAL x1, +256
INST_JAL_X1_256 = encode_j(0x6F, rd=1, imm21=256)


# ── Decode vectors ──────────────────────────────────────────────

DECODE_VECTORS: list[tuple[int, int, int, int, bool, bool, bool, bool, str]] = [
    # (inst, rd, rs1, rs2, is_branch, is_load, is_store, use_imm, desc)
    (INST_ADD_X1_X2_X3, 1, 2, 3, False, False, False, False, "ADD x1,x2,x3"),
    (INST_SUB_X4_X5_X6, 4, 5, 6, False, False, False, False, "SUB x4,x5,x6"),
    (INST_ADDI_X7_X8_42, 7, 8, 0, False, False, False, True, "ADDI x7,x8,42"),
    (INST_LW_X5_8_X10, 5, 10, 0, False, True, False, True, "LW x5,8(x10)"),
    (INST_SW_X3_16_X4, 0, 4, 3, False, False, True, True, "SW x3,16(x4)"),
    (INST_BEQ_X1_X2_8, 0, 1, 2, True, False, False, False, "BEQ x1,x2,+8"),
    (INST_LUI_X10, 10, 0, 0, False, False, False, True, "LUI x10,0xDEAD0"),
    (INST_JAL_X1_256, 1, 0, 0, True, False, False, True, "JAL x1,+256"),
]
