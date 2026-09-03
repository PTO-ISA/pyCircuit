"""Testbench for ALU — MLIR smoke (L1) + functional directed (L2).

L2 vectors cover every opcode with boundary / overflow / zero-flag cases.
The @testbench drives a 16-bit ALU so compilation stays fast.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.exu.alu import (  # noqa: E402
    OP_ADD,
    OP_AND,
    OP_OR,
    OP_SLL,
    OP_SLT,
    OP_SLTU,
    OP_SRA,
    OP_SRL,
    OP_SUB,
    OP_XOR,
    build_alu,
)

TEST_W = 16
MASK = (1 << TEST_W) - 1

# ── Golden vectors: (src1, src2, alu_op, expected_result, expected_zero) ──

_VECTORS: list[tuple[int, int, int, int, int]] = [
    # ADD
    (3, 5, OP_ADD, 8, 0),
    (0, 0, OP_ADD, 0, 1),
    (MASK, 1, OP_ADD, 0, 1),  # overflow wraps
    (0x7FFF, 1, OP_ADD, 0x8000, 0),  # signed overflow boundary
    # SUB
    (10, 3, OP_SUB, 7, 0),
    (5, 5, OP_SUB, 0, 1),
    (0, 1, OP_SUB, MASK, 0),  # underflow wraps
    # AND / OR / XOR
    (0xFF00, 0x0FF0, OP_AND, 0x0F00, 0),
    (0xFF00, 0x0FF0, OP_OR, 0xFFF0, 0),
    (0xFF00, 0x0FF0, OP_XOR, 0xF0F0, 0),
    (0xFFFF, 0xFFFF, OP_XOR, 0x0000, 1),  # self-XOR → 0
    # SLT (signed)
    (MASK, 1, OP_SLT, 1, 0),  # -1 < 1  → 1
    (1, MASK, OP_SLT, 0, 1),  # 1 < -1  → 0
    (0x8000, 0x7FFF, OP_SLT, 1, 0),  # min < max → 1
    (5, 5, OP_SLT, 0, 1),  # equal → 0
    # SLTU (unsigned)
    (1, MASK, OP_SLTU, 1, 0),  # 1 < 65535 → 1
    (MASK, 1, OP_SLTU, 0, 1),  # 65535 < 1 → 0
    (0, 1, OP_SLTU, 1, 0),  # 0 < 1 → 1
    (0, 0, OP_SLTU, 0, 1),  # equal → 0
    # SLL
    (1, 4, OP_SLL, 0x0010, 0),  # 1<<4 = 16
    (1, 15, OP_SLL, 0x8000, 0),  # 1<<15 = MSB
    (1, 0, OP_SLL, 1, 0),  # shift 0
    # SRL
    (0x8000, 1, OP_SRL, 0x4000, 0),
    (0x0001, 1, OP_SRL, 0, 1),  # shift out
    # SRA
    (0x8000, 1, OP_SRA, 0xC000, 0),  # sign-extended
    (0x4000, 1, OP_SRA, 0x2000, 0),  # positive stays positive
]


# ── L2: Functional testbench ────────────────────────────────────


@testbench
def tb_alu_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(len(_VECTORS) + 16)

    for i, (s1, s2, op, exp_r, exp_z) in enumerate(_VECTORS):
        if i > 0:
            tb.next()
        tb.drive("src1", s1 & MASK)
        tb.drive("src2", s2 & MASK)
        tb.drive("alu_op", op)
        tb.expect(
            "result", exp_r & MASK, msg=f"v{i}: op={op:#06b} 0x{s1:04x} ○ 0x{s2:04x}"
        )
        tb.expect("zero", exp_z, msg=f"v{i}: zero flag")

    tb.finish()


# ── L1: MLIR smoke (pytest) ─────────────────────────────────────


@pytest.mark.smoke
def test_alu_emit_mlir():
    mlir = compile_cycle_aware(
        build_alu,
        name="alu",
        eager=True,
        data_width=64,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "@alu" in mlir
    assert "result" in mlir


@pytest.mark.smoke
def test_alu_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_alu,
        name="alu_s",
        eager=True,
        data_width=TEST_W,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "result" in mlir


@pytest.mark.regcount
def test_alu_zero_registers():
    """ALU is purely combinational — MLIR must contain no pyc.reg."""
    import re

    mlir = compile_cycle_aware(
        build_alu,
        name="alu_rc",
        eager=True,
        data_width=TEST_W,
    ).emit_mlir()
    assert len(re.findall(r"pyc\.reg", mlir)) == 0, "ALU should have 0 registers"


if __name__ == "__main__":
    test_alu_small_emit_mlir()
    print("PASS: test_alu_small_emit_mlir")
    test_alu_emit_mlir()
    print("PASS: test_alu_emit_mlir")
    test_alu_zero_registers()
    print("PASS: test_alu_zero_registers")
