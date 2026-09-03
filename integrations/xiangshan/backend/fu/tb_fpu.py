"""Testbench for FPU — MLIR smoke (L1) + functional directed (L2).

L2 tests verify the 3-cycle pipelined path (FADD/FSUB/FMUL), the FSM-based
FDIV handshake (IDLE→BUSY→DONE), and flush behaviour.
Uses 16-bit / pipe_stages=3 / div_latency=4 for speed.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.fu.fpu import OP_FADD, OP_FDIV, OP_FMUL, OP_FSUB, build_fpu  # noqa: E402

TEST_W = 16
MASK = (1 << TEST_W) - 1
TEST_PIPE = 3
TEST_DIV_LAT = 4


@testbench
def tb_fpu_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(80)

    # ── Test 1: FADD pipeline latency ────────────────────────────
    tb.drive("in_valid", 1)
    tb.drive("src1", 100)
    tb.drive("src2", 200)
    tb.drive("fpu_op", OP_FADD)
    tb.drive("out_ready", 0)
    tb.drive("flush", 0)

    tb.expect("in_ready", 1, msg="T1: pipe → in_ready=1")
    tb.expect("out_valid", 0, msg="T1: no output yet at cycle 0")

    tb.next()
    tb.drive("in_valid", 0)
    for c in range(TEST_PIPE - 1):
        tb.expect("out_valid", 0, msg=f"T1: pipe stage {c}")
        tb.next()

    tb.expect("out_valid", 1, msg="T1: pipe done after 3 cycles")
    tb.expect("result", (100 + 200) & MASK, msg="T1: FADD result")

    # ── Test 2: FDIV FSM handshake (IDLE→BUSY→DONE→IDLE) ────────
    tb.next()
    tb.drive("in_valid", 1)
    tb.drive("src1", 400)
    tb.drive("src2", 20)
    tb.drive("fpu_op", OP_FDIV)
    tb.drive("out_ready", 0)
    tb.drive("flush", 0)

    tb.expect("in_ready", 1, msg="T2: idle → in_ready=1")
    tb.expect("out_valid", 0, msg="T2: idle → out_valid=0")

    tb.next()
    tb.drive("in_valid", 0)
    tb.expect("in_ready", 0, msg="T2: busy → in_ready=0")
    tb.expect("out_valid", 0, msg="T2: busy → out_valid=0")

    for c in range(TEST_DIV_LAT - 1):
        tb.next()
        tb.expect("out_valid", 0, msg=f"T2: busy tick {c}")

    tb.next()
    tb.expect("out_valid", 1, msg="T2: done → out_valid=1")
    tb.expect("in_ready", 0, msg="T2: done → in_ready=0")

    tb.drive("out_ready", 1)
    tb.next()
    tb.drive("out_ready", 0)
    tb.expect("in_ready", 1, msg="T2: back to idle")
    tb.expect("out_valid", 0, msg="T2: idle → out_valid=0")

    # ── Test 3: Flush during FADD pipeline ───────────────────────
    tb.next()
    tb.drive("in_valid", 1)
    tb.drive("src1", 50)
    tb.drive("src2", 60)
    tb.drive("fpu_op", OP_FADD)
    tb.drive("flush", 0)

    tb.next()
    tb.drive("in_valid", 0)

    tb.next()

    tb.drive("flush", 1)
    tb.next()
    tb.expect("out_valid", 0, msg="T3: flush gates pipe output")

    tb.drive("flush", 0)
    tb.next()
    tb.expect("out_valid", 0, msg="T3: pipe drained after flush")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_fpu_emit_mlir():
    mlir = compile_cycle_aware(
        build_fpu,
        name="fpu",
        eager=True,
        data_width=64,
        pipe_latency=3,
        fdiv_latency=12,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "@fpu" in mlir
    assert "result" in mlir


@pytest.mark.smoke
def test_fpu_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_fpu,
        name="fpu_s",
        eager=True,
        data_width=TEST_W,
        pipe_latency=TEST_PIPE,
        fdiv_latency=TEST_DIV_LAT,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "result" in mlir


@pytest.mark.regcount
def test_fpu_has_state_regs():
    import re

    mlir = compile_cycle_aware(
        build_fpu,
        name="fpu_rc",
        eager=True,
        data_width=TEST_W,
        pipe_latency=TEST_PIPE,
        fdiv_latency=TEST_DIV_LAT,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    assert n >= 8, f"FPU must have ≥8 state regs (pipe + FSM); got {n}"


if __name__ == "__main__":
    test_fpu_small_emit_mlir()
    print("PASS: test_fpu_small_emit_mlir")
    test_fpu_emit_mlir()
    print("PASS: test_fpu_emit_mlir")
    test_fpu_has_state_regs()
    print("PASS: test_fpu_has_state_regs")
