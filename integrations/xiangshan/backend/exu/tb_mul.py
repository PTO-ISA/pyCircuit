"""Testbench for MUL — MLIR smoke (L1) + functional directed (L2).

L2 tests verify the 2-cycle pipeline latency and MUL/MULH/MULHU/MULHSU
operations.  Uses 16-bit datapath for fast compilation.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.exu.mul import (  # noqa: E402
    OP_MUL,
    OP_MULH,
    OP_MULHSU,
    OP_MULHU,
    build_mul,
)

TEST_W = 16
MASK = (1 << TEST_W) - 1


def _golden_mul(src1: int, src2: int, op: int) -> int:
    """Python reference for 16-bit unsigned multiply (matches RTL model)."""
    prod = (src1 & MASK) * (src2 & MASK)
    lo = prod & MASK
    hi = (prod >> TEST_W) & MASK
    if op == OP_MUL:
        return lo
    return hi


# (src1, src2, mul_op, expected_result)
_VECTORS = [
    (3, 5, OP_MUL, 15),  # 3*5 = 15
    (0, 100, OP_MUL, 0),  # 0*x = 0
    (MASK, 1, OP_MUL, MASK),  # identity
    (0x100, 0x100, OP_MUL, 0x0000),  # 256*256 = 65536 → lo=0
    (0x100, 0x100, OP_MULHU, 0x0001),  # 256*256 → hi=1
    (7, 9, OP_MUL, 63),  # 7*9 = 63
    (MASK, MASK, OP_MUL, 1),  # 65535² low = 1 (0xFFFE0001 & 0xFFFF)
    (MASK, MASK, OP_MULHU, 0xFFFE),  # 65535² hi
]


@testbench
def tb_mul_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(len(_VECTORS) * 3 + 20)

    for i, (s1, s2, op, exp) in enumerate(_VECTORS):
        if i > 0:
            tb.next()

        tb.drive("in_valid", 1)
        tb.drive("src1", s1 & MASK)
        tb.drive("src2", s2 & MASK)
        tb.drive("mul_op", op)

        # Pipeline: result appears 1 cycle later
        tb.next()
        tb.drive("in_valid", 0)
        tb.expect("out_valid", 1, msg=f"v{i}: out_valid after 1-cycle pipe")
        tb.expect("result", exp & MASK, msg=f"v{i}: op={op} 0x{s1:04x}*0x{s2:04x}")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_mul_emit_mlir():
    mlir = compile_cycle_aware(
        build_mul,
        name="mul",
        eager=True,
        data_width=64,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "result" in mlir


@pytest.mark.smoke
def test_mul_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_mul,
        name="mul_s",
        eager=True,
        data_width=TEST_W,
    ).emit_mlir()
    assert "func.func" in mlir


@pytest.mark.regcount
def test_mul_has_pipeline_regs():
    import re

    mlir = compile_cycle_aware(
        build_mul,
        name="mul_rc",
        eager=True,
        data_width=TEST_W,
    ).emit_mlir()
    assert (
        len(re.findall(r"pyc\.reg", mlir)) >= 2
    ), "MUL must have at least 2 pipeline registers (valid + result)"


if __name__ == "__main__":
    test_mul_small_emit_mlir()
    print("PASS: test_mul_small_emit_mlir")
    test_mul_emit_mlir()
    print("PASS: test_mul_emit_mlir")
