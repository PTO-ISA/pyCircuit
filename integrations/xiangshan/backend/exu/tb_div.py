"""Testbench for DIV — MLIR smoke (L1) + functional directed (L2).

L2 tests verify the FSM handshake (IDLE → BUSY → DONE → IDLE) and
division-by-zero behaviour.  Uses 16-bit / latency=4 for speed.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.exu.div import OP_DIV, OP_DIVU, OP_REM, OP_REMU, build_div  # noqa: E402

TEST_W = 16
MASK = (1 << TEST_W) - 1
TEST_LAT = 4


@testbench
def tb_div_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(80)

    # ── Test 1: basic FSM handshake ──────────────────────────────
    # Drive valid input at cycle 0
    tb.drive("in_valid", 1)
    tb.drive("src1", 100)
    tb.drive("src2", 10)
    tb.drive("div_op", OP_DIVU)
    tb.drive("out_ready", 0)
    tb.drive("flush", 0)

    tb.expect("in_ready", 1, msg="T1: idle → in_ready=1")
    tb.expect("out_valid", 0, msg="T1: idle → out_valid=0")

    # Cycle 1: should transition to BUSY, in_ready drops
    tb.next()
    tb.drive("in_valid", 0)
    tb.expect("in_ready", 0, msg="T1: busy → in_ready=0")
    tb.expect("out_valid", 0, msg="T1: busy → out_valid=0")

    # Wait through BUSY period (latency-1 counter counts down)
    for c in range(TEST_LAT - 1):
        tb.next()
        tb.expect("out_valid", 0, msg=f"T1: busy tick {c}")

    # After latency cycles total, should be DONE
    tb.next()
    tb.expect("out_valid", 1, msg="T1: done → out_valid=1")
    tb.expect("in_ready", 0, msg="T1: done → in_ready=0 (busy)")

    # Acknowledge result
    tb.drive("out_ready", 1)
    tb.next()
    tb.drive("out_ready", 0)

    # Should return to IDLE
    tb.expect("in_ready", 1, msg="T1: back to idle")
    tb.expect("out_valid", 0, msg="T1: idle → out_valid=0")

    # ── Test 2: division by zero ─────────────────────────────────
    tb.next()
    tb.drive("in_valid", 1)
    tb.drive("src1", 42)
    tb.drive("src2", 0)  # divisor = 0
    tb.drive("div_op", OP_DIVU)
    tb.drive("flush", 0)
    tb.expect("in_ready", 1, msg="T2: start div-by-zero")

    tb.next()
    tb.drive("in_valid", 0)

    for _ in range(TEST_LAT):
        tb.next()

    tb.expect("out_valid", 1, msg="T2: div-by-zero done")

    tb.drive("out_ready", 1)
    tb.next()
    tb.drive("out_ready", 0)
    tb.expect("in_ready", 1, msg="T2: back to idle")

    # ── Test 3: flush during BUSY ────────────────────────────────
    tb.next()
    tb.drive("in_valid", 1)
    tb.drive("src1", 200)
    tb.drive("src2", 7)
    tb.drive("div_op", OP_DIV)
    tb.drive("flush", 0)

    tb.next()
    tb.drive("in_valid", 0)

    # flush mid-operation
    tb.next()
    tb.drive("flush", 1)
    tb.next()
    tb.drive("flush", 0)
    tb.expect("in_ready", 1, msg="T3: flush → back to idle")
    tb.expect("out_valid", 0, msg="T3: flush → no valid output")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_div_emit_mlir():
    mlir = compile_cycle_aware(
        build_div,
        name="div",
        eager=True,
        data_width=64,
        latency=8,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "result" in mlir


@pytest.mark.smoke
def test_div_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_div,
        name="div_s",
        eager=True,
        data_width=TEST_W,
        latency=TEST_LAT,
    ).emit_mlir()
    assert "func.func" in mlir


@pytest.mark.regcount
def test_div_has_state_regs():
    import re

    mlir = compile_cycle_aware(
        build_div,
        name="div_rc",
        eager=True,
        data_width=TEST_W,
        latency=TEST_LAT,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    assert n >= 5, f"DIV must have ≥5 state regs (fsm,cnt,s1,s2,op,result); got {n}"


if __name__ == "__main__":
    test_div_small_emit_mlir()
    print("PASS: test_div_small_emit_mlir")
    test_div_emit_mlir()
    print("PASS: test_div_emit_mlir")
