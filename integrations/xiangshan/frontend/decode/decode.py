"""Decode — Instruction Decode Unit for XiangShan-pyc.

Takes up to ``decode_width`` instructions from IBuffer and produces decoded
micro-ops with control signals.

Pipeline:
  Cycle 0  Pure combinational decode (field extraction, type detection,
           immediate generation, control signal generation)
  Cycle 1  Registered output (pipeline register to downstream rename stage)

Reference: XiangShan/src/main/scala/xiangshan/backend/decode/

Key features:
  F-DEC-001  RV64IMAFDC field extraction (opcode, rd, rs1, rs2, funct3, funct7)
  F-DEC-002  Instruction type detection (R / I / S / B / U / J)
  F-DEC-003  Immediate extraction and sign extension for all types
  F-DEC-004  Control signal generation (branch, load, store, alu, imm select, …)
  F-DEC-005  FP / Vec detection (simplified)
  F-DEC-006  Registered output pipeline stage
"""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    cas,
    wire_of,
)
from top.parameters import DECODE_WIDTH, PC_WIDTH

INST_WIDTH = 32
IMM_WIDTH = 32

# ── RISC-V base opcode constants (bits [6:0]) ────────────────────────
OP_LUI = 0x37  # 0110111
OP_AUIPC = 0x17  # 0010111
OP_JAL = 0x6F  # 1101111
OP_JALR = 0x67  # 1100111
OP_BRANCH = 0x63  # 1100011
OP_LOAD = 0x03  # 0000011
OP_STORE = 0x23  # 0100011
OP_OP_IMM = 0x13  # 0010011
OP_OP = 0x33  # 0110011
OP_OP_IMM_W = 0x1B  # 0011011  (RV64 word-immediate)
OP_OP_W = 0x3B  # 0111011  (RV64 word-register)
OP_SYSTEM = 0x73  # 1110011
OP_FENCE = 0x0F  # 0001111

# FP / Vec opcodes (simplified detection)
OP_FP_LOAD = 0x07  # 0000111
OP_FP_STORE = 0x27  # 0100111
OP_FMADD = 0x43  # 1000011
OP_FMSUB = 0x47  # 1000111
OP_FNMSUB = 0x4B  # 1001011
OP_FNMADD = 0x4F  # 1001111
OP_FP_OP = 0x53  # 1010011
OP_VEC = 0x57  # 1010111


def _sext_wire(m, val, from_w: int, to_w: int):
    """Sign-extend a *wire* from ``from_w`` bits to ``to_w`` bits."""
    if from_w >= to_w:
        return val[0:to_w]
    sign = val[from_w - 1 : from_w]
    ext_w = to_w - from_w
    ext = sign.select(
        m.const((1 << ext_w) - 1, width=ext_w),
        m.const(0, width=ext_w),
    )
    return m.cat(ext, val)


def decode(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "dec",
    decode_width: int = DECODE_WIDTH,
    pc_width: int = PC_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """Decode: combinational RISC-V instruction decoder with registered output."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    flush = (
        _in["flush"]
        if "flush" in _in
        else cas(domain, m.input(f"{prefix}_flush", width=1), cycle=0)
    )

    reg_out: dict[str, object] = {}

    # ── Cycle 0: combinational decode ─────────────────────────────────

    for i in range(decode_width):
        in_valid = cas(domain, m.input(f"{prefix}_in_valid_{i}", width=1), cycle=0)
        in_inst = cas(
            domain, m.input(f"{prefix}_in_inst_{i}", width=INST_WIDTH), cycle=0
        )
        in_pc = cas(domain, m.input(f"{prefix}_in_pc_{i}", width=pc_width), cycle=0)
        cas(domain, m.input(f"{prefix}_in_is_rvc_{i}", width=1), cycle=0)

        inst = wire_of(in_inst)

        # ── Fixed-position field extraction ──
        opcode = inst[0:7]
        rd = inst[7:12]
        funct3 = inst[12:15]
        rs1 = inst[15:20]
        rs2 = inst[20:25]
        funct7 = inst[25:32]

        # ── Instruction type detection ──
        def _c(v):
            return m.const(v, width=7)

        is_lui = opcode == _c(OP_LUI)
        is_auipc = opcode == _c(OP_AUIPC)
        is_jal = opcode == _c(OP_JAL)
        is_jalr = opcode == _c(OP_JALR)
        is_branch = opcode == _c(OP_BRANCH)
        is_load = opcode == _c(OP_LOAD)
        is_store = opcode == _c(OP_STORE)
        is_op_imm = opcode == _c(OP_OP_IMM)
        is_op = opcode == _c(OP_OP)
        is_op_imm_w = opcode == _c(OP_OP_IMM_W)
        is_op_w = opcode == _c(OP_OP_W)
        opcode == _c(OP_SYSTEM)

        # FP / Vec
        is_fp = (
            (opcode == _c(OP_FP_LOAD))
            | (opcode == _c(OP_FP_STORE))
            | (opcode == _c(OP_FMADD))
            | (opcode == _c(OP_FMSUB))
            | (opcode == _c(OP_FNMSUB))
            | (opcode == _c(OP_FNMADD))
            | (opcode == _c(OP_FP_OP))
        )
        is_vec = opcode == _c(OP_VEC)

        # Aggregate type categories
        is_r_type = is_op | is_op_w
        is_i_type = is_op_imm | is_op_imm_w | is_load | is_jalr
        is_s_type = is_store
        is_b_type = is_branch
        is_u_type = is_lui | is_auipc
        is_j_type = is_jal

        # ── Immediate extraction ──

        # I-type: inst[31:20] → 12 bits, sign-extended to 32
        imm_i = _sext_wire(m, inst[20:32], 12, IMM_WIDTH)

        # S-type: {inst[31:25], inst[11:7]} → 12 bits, sign-extended
        imm_s = _sext_wire(m, m.cat(inst[25:32], inst[7:12]), 12, IMM_WIDTH)

        # B-type: {inst[31], inst[7], inst[30:25], inst[11:8], 0} → 13 bits
        imm_b_raw = m.cat(
            inst[31:32],
            inst[7:8],
            inst[25:31],
            inst[8:12],
            m.const(0, width=1),
        )
        imm_b = _sext_wire(m, imm_b_raw, 13, IMM_WIDTH)

        # U-type: {inst[31:12], 12'b0} → 32 bits
        imm_u = m.cat(inst[12:32], m.const(0, width=12))

        # J-type: {inst[31], inst[19:12], inst[20], inst[30:21], 0} → 21 bits
        imm_j_raw = m.cat(
            inst[31:32],
            inst[12:20],
            inst[20:21],
            inst[21:31],
            m.const(0, width=1),
        )
        imm_j = _sext_wire(m, imm_j_raw, 21, IMM_WIDTH)

        # Priority-select immediate by instruction type
        imm = imm_i
        imm = is_s_type.select(imm_s, imm)
        imm = is_b_type.select(imm_b, imm)
        imm = is_u_type.select(imm_u, imm)
        imm = is_j_type.select(imm_j, imm)

        # ── Control signal generation ──

        use_imm = is_i_type | is_s_type | is_b_type | is_u_type | is_j_type
        rd_valid = is_r_type | is_i_type | is_u_type | is_j_type
        rs1_valid = is_r_type | is_i_type | is_s_type | is_b_type
        rs2_valid = is_r_type | is_s_type | is_b_type

        dec_valid = in_valid & (~flush)

        # ── Pipeline registers (cycle 0 → cycle 1) ──
        tag = f"d{i}"
        reg_out[f"v_{i}"] = domain.cycle(wire_of(dec_valid), name=f"{prefix}_{tag}_v")
        reg_out[f"pc_{i}"] = domain.cycle(wire_of(in_pc), name=f"{prefix}_{tag}_pc")
        reg_out[f"inst_{i}"] = domain.cycle(inst, name=f"{prefix}_{tag}_inst")
        reg_out[f"rd_{i}"] = domain.cycle(rd, name=f"{prefix}_{tag}_rd")
        reg_out[f"rs1_{i}"] = domain.cycle(rs1, name=f"{prefix}_{tag}_rs1")
        reg_out[f"rs2_{i}"] = domain.cycle(rs2, name=f"{prefix}_{tag}_rs2")
        reg_out[f"funct3_{i}"] = domain.cycle(funct3, name=f"{prefix}_{tag}_f3")
        reg_out[f"funct7_{i}"] = domain.cycle(funct7, name=f"{prefix}_{tag}_f7")
        reg_out[f"imm_{i}"] = domain.cycle(imm, name=f"{prefix}_{tag}_imm")
        reg_out[f"alu_op_{i}"] = domain.cycle(funct3, name=f"{prefix}_{tag}_alu")
        reg_out[f"is_branch_{i}"] = domain.cycle(is_branch, name=f"{prefix}_{tag}_br")
        reg_out[f"is_jal_{i}"] = domain.cycle(is_jal, name=f"{prefix}_{tag}_jal")
        reg_out[f"is_jalr_{i}"] = domain.cycle(is_jalr, name=f"{prefix}_{tag}_jalr")
        reg_out[f"is_load_{i}"] = domain.cycle(is_load, name=f"{prefix}_{tag}_ld")
        reg_out[f"is_store_{i}"] = domain.cycle(is_store, name=f"{prefix}_{tag}_st")
        reg_out[f"use_imm_{i}"] = domain.cycle(use_imm, name=f"{prefix}_{tag}_uimm")
        reg_out[f"rd_valid_{i}"] = domain.cycle(rd_valid, name=f"{prefix}_{tag}_rdv")
        reg_out[f"rs1_valid_{i}"] = domain.cycle(rs1_valid, name=f"{prefix}_{tag}_r1v")
        reg_out[f"rs2_valid_{i}"] = domain.cycle(rs2_valid, name=f"{prefix}_{tag}_r2v")
        reg_out[f"is_fp_{i}"] = domain.cycle(is_fp, name=f"{prefix}_{tag}_fp")
        reg_out[f"is_vec_{i}"] = domain.cycle(is_vec, name=f"{prefix}_{tag}_vec")

    # ── Cycle 1: emit registered outputs ──────────────────────────────

    domain.next()

    for i in range(decode_width):
        m.output(f"{prefix}_out_valid_{i}", reg_out[f"v_{i}"])
        m.output(f"{prefix}_out_pc_{i}", reg_out[f"pc_{i}"])
        m.output(f"{prefix}_out_inst_{i}", reg_out[f"inst_{i}"])
        m.output(f"{prefix}_out_rd_{i}", reg_out[f"rd_{i}"])
        m.output(f"{prefix}_out_rs1_{i}", reg_out[f"rs1_{i}"])
        m.output(f"{prefix}_out_rs2_{i}", reg_out[f"rs2_{i}"])
        m.output(f"{prefix}_out_funct3_{i}", reg_out[f"funct3_{i}"])
        m.output(f"{prefix}_out_funct7_{i}", reg_out[f"funct7_{i}"])
        m.output(f"{prefix}_out_imm_{i}", reg_out[f"imm_{i}"])
        m.output(f"{prefix}_out_alu_op_{i}", reg_out[f"alu_op_{i}"])
        m.output(f"{prefix}_out_is_branch_{i}", reg_out[f"is_branch_{i}"])
        m.output(f"{prefix}_out_is_jal_{i}", reg_out[f"is_jal_{i}"])
        m.output(f"{prefix}_out_is_jalr_{i}", reg_out[f"is_jalr_{i}"])
        m.output(f"{prefix}_out_is_load_{i}", reg_out[f"is_load_{i}"])
        m.output(f"{prefix}_out_is_store_{i}", reg_out[f"is_store_{i}"])
        m.output(f"{prefix}_out_use_imm_{i}", reg_out[f"use_imm_{i}"])
        m.output(f"{prefix}_out_rd_valid_{i}", reg_out[f"rd_valid_{i}"])
        m.output(f"{prefix}_out_rs1_valid_{i}", reg_out[f"rs1_valid_{i}"])
        m.output(f"{prefix}_out_rs2_valid_{i}", reg_out[f"rs2_valid_{i}"])
        m.output(f"{prefix}_out_is_fp_{i}", reg_out[f"is_fp_{i}"])
        m.output(f"{prefix}_out_is_vec_{i}", reg_out[f"is_vec_{i}"])
    return _out


decode.__pycircuit_name__ = "decode"


if __name__ == "__main__":
    pass
