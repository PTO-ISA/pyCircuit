"""Testbench for Decode — MLIR smoke (L1) + functional directed (L2).

L2 tests verify the 2-stage decode pipeline: combinational field extraction
at cycle 0, registered output at cycle 1, and flush gating.
Uses decode_width=2, pc_width=16 for speed.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from frontend.decode.decode import build_decode  # noqa: E402

TEST_DW = 2
TEST_PW = 16

# ── RISC-V test instruction encodings ────────────────────────────

# ADD x1, x2, x3 — R-type (opcode=0x33, funct3=0, funct7=0)
INST_ADD = (0 << 25) | (3 << 20) | (2 << 15) | (0 << 12) | (1 << 7) | 0x33

# LW x5, 8(x10) — I-type LOAD (opcode=0x03, funct3=2, imm=8)
INST_LW = (8 << 20) | (10 << 15) | (2 << 12) | (5 << 7) | 0x03


@testbench
def tb_decode_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(30)

    # ── Test 1: Empty after reset — no valid outputs ─────────────
    tb.drive("flush", 0)
    for i in range(TEST_DW):
        tb.drive(f"in_valid_{i}", 0)
        tb.drive(f"in_inst_{i}", 0)
        tb.drive(f"in_pc_{i}", 0)
        tb.drive(f"in_is_rvc_{i}", 0)

    tb.next()
    for i in range(TEST_DW):
        tb.expect(f"out_valid_{i}", 0, msg=f"T1: slot {i} invalid after reset")

    # ── Test 2: Decode valid instructions, outputs 1 cycle later ──
    tb.drive("in_valid_0", 1)
    tb.drive("in_inst_0", INST_ADD)
    tb.drive("in_pc_0", 0x100)
    tb.drive("in_is_rvc_0", 0)

    tb.drive("in_valid_1", 1)
    tb.drive("in_inst_1", INST_LW)
    tb.drive("in_pc_1", 0x104)
    tb.drive("in_is_rvc_1", 0)

    tb.next()
    tb.drive("in_valid_0", 0)
    tb.drive("in_valid_1", 0)

    # Slot 0: ADD x1, x2, x3 (R-type)
    tb.expect("out_valid_0", 1, msg="T2: ADD valid")
    tb.expect("out_rd_0", 1, msg="T2: ADD rd=x1")
    tb.expect("out_rs1_0", 2, msg="T2: ADD rs1=x2")
    tb.expect("out_rs2_0", 3, msg="T2: ADD rs2=x3")
    tb.expect("out_rd_valid_0", 1, msg="T2: ADD rd_valid (R-type)")
    tb.expect("out_rs1_valid_0", 1, msg="T2: ADD rs1_valid (R-type)")
    tb.expect("out_rs2_valid_0", 1, msg="T2: ADD rs2_valid (R-type)")
    tb.expect("out_use_imm_0", 0, msg="T2: ADD use_imm=0 (R-type)")
    tb.expect("out_is_load_0", 0, msg="T2: ADD not load")
    tb.expect("out_is_branch_0", 0, msg="T2: ADD not branch")
    tb.expect("out_pc_0", 0x100, msg="T2: ADD pc passthrough")

    # Slot 1: LW x5, 8(x10) (I-type LOAD)
    tb.expect("out_valid_1", 1, msg="T2: LW valid")
    tb.expect("out_rd_1", 5, msg="T2: LW rd=x5")
    tb.expect("out_rs1_1", 10, msg="T2: LW rs1=x10")
    tb.expect("out_rd_valid_1", 1, msg="T2: LW rd_valid (I-type)")
    tb.expect("out_rs1_valid_1", 1, msg="T2: LW rs1_valid (I-type)")
    tb.expect("out_rs2_valid_1", 0, msg="T2: LW rs2_valid=0 (I-type)")
    tb.expect("out_use_imm_1", 1, msg="T2: LW use_imm=1 (I-type)")
    tb.expect("out_is_load_1", 1, msg="T2: LW is_load")
    tb.expect("out_is_branch_1", 0, msg="T2: LW not branch")
    tb.expect("out_pc_1", 0x104, msg="T2: LW pc passthrough")

    # ── Test 3: Flush clears pipeline ────────────────────────────
    tb.next()

    tb.drive("in_valid_0", 1)
    tb.drive("in_inst_0", INST_ADD)
    tb.drive("in_pc_0", 0x200)
    tb.drive("in_valid_1", 1)
    tb.drive("in_inst_1", INST_LW)
    tb.drive("in_pc_1", 0x204)
    tb.drive("flush", 1)

    tb.next()
    tb.drive("flush", 0)
    tb.drive("in_valid_0", 0)
    tb.drive("in_valid_1", 0)
    tb.expect("out_valid_0", 0, msg="T3: flush killed slot 0")
    tb.expect("out_valid_1", 0, msg="T3: flush killed slot 1")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_decode_emit_mlir():
    from top.parameters import DECODE_WIDTH, PC_WIDTH

    mlir = compile_cycle_aware(
        build_decode,
        name="decode",
        eager=True,
        decode_width=DECODE_WIDTH,
        pc_width=PC_WIDTH,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "@decode" in mlir
    assert "out_valid_0" in mlir


@pytest.mark.smoke
def test_decode_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_decode,
        name="decode_s",
        eager=True,
        decode_width=TEST_DW,
        pc_width=TEST_PW,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "out_valid_0" in mlir


@pytest.mark.regcount
def test_decode_has_pipeline_regs():
    import re

    mlir = compile_cycle_aware(
        build_decode,
        name="decode_rc",
        eager=True,
        decode_width=TEST_DW,
        pc_width=TEST_PW,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    assert (
        n >= 20
    ), f"Decode must have ≥20 pipeline registers (2 slots × fields); got {n}"


if __name__ == "__main__":
    test_decode_small_emit_mlir()
    print("PASS: test_decode_small_emit_mlir")
    test_decode_emit_mlir()
    print("PASS: test_decode_emit_mlir")
    test_decode_has_pipeline_regs()
    print("PASS: test_decode_has_pipeline_regs")
