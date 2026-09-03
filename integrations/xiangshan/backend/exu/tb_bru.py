"""Testbench for BRU — MLIR smoke (L1) + functional directed (L2).

L2 vectors cover every branch type, mispredict detection, and target
address computation for both 16-bit fast and 64-bit smoke configs.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.exu.bru import (  # noqa: E402
    OP_BEQ,
    OP_BGE,
    OP_BGEU,
    OP_BLT,
    OP_BLTU,
    OP_BNE,
    OP_JAL,
    OP_JALR,
    build_bru,
)

TEST_W = 16
PC_W = 16
MASK = (1 << TEST_W) - 1
PC_MASK = (1 << PC_W) - 1


def _golden_taken(src1: int, src2: int, op: int) -> int:
    """Compute expected taken bit using Python arithmetic (signed 16-bit)."""
    s1 = src1 if src1 < 0x8000 else src1 - 0x10000
    s2 = src2 if src2 < 0x8000 else src2 - 0x10000
    if op == OP_BEQ:
        return int(src1 == src2)
    if op == OP_BNE:
        return int(src1 != src2)
    if op == OP_BLT:
        return int(s1 < s2)
    if op == OP_BGE:
        return int(s1 >= s2)
    if op == OP_BLTU:
        return int(src1 < src2)
    if op == OP_BGEU:
        return int(src1 >= src2)
    if op in (OP_JAL, OP_JALR):
        return 1
    return 0


def _golden_target(src1: int, pc: int, imm: int, op: int) -> int:
    if op == OP_JALR:
        return ((src1 + imm) & ~1) & PC_MASK
    return (pc + imm) & PC_MASK


# (src1, src2, pc, imm, op, predicted_taken)
_VECTORS = [
    # BEQ
    (5, 5, 0x100, 0x10, OP_BEQ, 0),  # equal → taken, predicted NT → mispredict
    (5, 6, 0x100, 0x10, OP_BEQ, 0),  # not equal → NT, predicted NT → correct
    # BNE
    (5, 6, 0x200, 0x20, OP_BNE, 1),  # not equal → taken, predicted T → correct
    (5, 5, 0x200, 0x20, OP_BNE, 1),  # equal → NT, predicted T → mispredict
    # BLT (signed)
    (MASK, 1, 0x300, 0x08, OP_BLT, 0),  # -1 < 1 → taken
    (1, MASK, 0x300, 0x08, OP_BLT, 0),  # 1 < -1 → NT
    # BGE (signed)
    (1, MASK, 0x400, 0x04, OP_BGE, 0),  # 1 >= -1 → taken
    (MASK, 1, 0x400, 0x04, OP_BGE, 0),  # -1 >= 1 → NT
    # BLTU (unsigned)
    (1, MASK, 0x500, 0x0C, OP_BLTU, 0),  # 1 < 65535 → taken
    (MASK, 1, 0x500, 0x0C, OP_BLTU, 0),  # 65535 < 1 → NT
    # BGEU (unsigned)
    (MASK, 1, 0x600, 0x10, OP_BGEU, 0),  # 65535 >= 1 → taken
    (1, MASK, 0x600, 0x10, OP_BGEU, 1),  # 1 >= 65535 → NT, predicted T → mispredict
    # JAL
    (0, 0, 0x1000, 0x100, OP_JAL, 1),  # always taken
    (0, 0, 0x1000, 0x100, OP_JAL, 0),  # predicted NT → mispredict
    # JALR
    (0x2000, 0, 0x1000, 0x10, OP_JALR, 1),  # target=(src1+imm)&~1
    (0x2001, 0, 0x1000, 0x10, OP_JALR, 1),  # odd src1: bit 0 cleared
]


@testbench
def tb_bru_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(len(_VECTORS) + 16)

    for i, (s1, s2, pc, imm, op, pred) in enumerate(_VECTORS):
        if i > 0:
            tb.next()
        tb.drive("in_valid", 1)
        tb.drive("src1", s1 & MASK)
        tb.drive("src2", s2 & MASK)
        tb.drive("pc", pc & PC_MASK)
        tb.drive("imm", imm & MASK)
        tb.drive("bru_op", op)
        tb.drive("predicted_taken", pred)

        exp_taken = _golden_taken(s1 & MASK, s2 & MASK, op)
        exp_target = _golden_target(s1 & MASK, pc & PC_MASK, imm & MASK, op)
        exp_link = (pc + 4) & PC_MASK
        exp_mispredict = int(exp_taken != pred)
        exp_redirect = exp_mispredict  # redirect_valid = in_valid & mispredict

        tb.expect("taken", exp_taken, msg=f"v{i}: taken")
        tb.expect("target", exp_target, msg=f"v{i}: target")
        tb.expect("mispredict", exp_mispredict, msg=f"v{i}: mispredict")
        tb.expect("redirect_valid", exp_redirect, msg=f"v{i}: redirect")
        tb.expect("link_addr", exp_link, msg=f"v{i}: link")
        is_link = int(op in (OP_JAL, OP_JALR))
        tb.expect("is_link", is_link, msg=f"v{i}: is_link")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_bru_emit_mlir():
    mlir = compile_cycle_aware(
        build_bru,
        name="bru",
        eager=True,
        data_width=64,
        pc_width=39,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "taken" in mlir
    assert "redirect_valid" in mlir


@pytest.mark.smoke
def test_bru_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_bru,
        name="bru_s",
        eager=True,
        data_width=TEST_W,
        pc_width=PC_W,
    ).emit_mlir()
    assert "func.func" in mlir


@pytest.mark.regcount
def test_bru_zero_registers():
    import re

    mlir = compile_cycle_aware(
        build_bru,
        name="bru_rc",
        eager=True,
        data_width=TEST_W,
        pc_width=PC_W,
    ).emit_mlir()
    assert len(re.findall(r"pyc\.reg", mlir)) == 0, "BRU should have 0 registers"


if __name__ == "__main__":
    test_bru_small_emit_mlir()
    print("PASS: test_bru_small_emit_mlir")
    test_bru_emit_mlir()
    print("PASS: test_bru_emit_mlir")
