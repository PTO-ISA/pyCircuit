"""Testbench for IBuffer — MLIR smoke (L1) + functional directed (L2).

L2 scenarios:
  T1  Empty after reset — num_valid=0, in_ready=1
  T2  Enqueue 2 instructions — check num_valid, out_valid
  T3  Dequeue — decode_accept, pointers advance
  T4  Fill to capacity → backpressure (in_ready=0)
  T5  Flush — pointers reset, num_valid=0

Uses tiny config: size=4, enq_width=2, deq_width=2, 16-bit inst/pc.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from frontend.ibuffer.ibuffer import build_ibuffer  # noqa: E402

SIZE = 4
ENQ_W = 2
DEQ_W = 2
INST_W = 16
PC_W = 16


def _zero_inputs(tb: CycleAwareTb) -> None:
    tb.drive("flush", 0)
    tb.drive("in_valid", 0)
    tb.drive("in_num", 0)
    for i in range(ENQ_W):
        tb.drive(f"in_inst_{i}", 0)
        tb.drive(f"in_pc_{i}", 0)
        tb.drive(f"in_is_rvc_{i}", 0)
    tb.drive("decode_accept", 0)


@testbench
def tb_ibuffer_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(40)

    # ── T1: empty after reset ────────────────────────────────────
    _zero_inputs(tb)
    tb.expect("num_valid", 0, msg="T1: empty after reset")
    tb.expect("in_ready", 1, msg="T1: ready when empty")
    for i in range(DEQ_W):
        tb.expect(f"out_valid_{i}", 0, msg=f"T1: out_valid_{i}=0")

    # ── T2: enqueue 2 instructions ───────────────────────────────
    tb.next()
    _zero_inputs(tb)
    tb.drive("in_valid", 1)
    tb.drive("in_num", 2)
    tb.drive("in_inst_0", 0x1001)
    tb.drive("in_pc_0", 0x1000)
    tb.drive("in_inst_1", 0x2002)
    tb.drive("in_pc_1", 0x1004)
    tb.expect("in_ready", 1, msg="T2: ready (space available)")

    # After the clock edge, entries are written
    tb.next()
    _zero_inputs(tb)
    tb.expect("num_valid", 2, msg="T2: 2 entries")
    tb.expect("out_valid_0", 1, msg="T2: out_valid_0=1")
    tb.expect("out_valid_1", 1, msg="T2: out_valid_1=1")
    tb.expect("out_inst_0", 0x1001, msg="T2: inst0 data")
    tb.expect("out_inst_1", 0x2002, msg="T2: inst1 data")

    # ── T3: dequeue by asserting decode_accept ───────────────────
    tb.drive("decode_accept", 1)

    tb.next()
    _zero_inputs(tb)
    tb.expect("num_valid", 0, msg="T3: drained after accept")
    tb.expect("out_valid_0", 0, msg="T3: no valid output")

    # ── T4: fill to capacity (4 entries via 2 + 2) ───────────────
    tb.next()
    _zero_inputs(tb)
    tb.drive("in_valid", 1)
    tb.drive("in_num", 2)
    tb.drive("in_inst_0", 0xA001)
    tb.drive("in_pc_0", 0x2000)
    tb.drive("in_inst_1", 0xA002)
    tb.drive("in_pc_1", 0x2004)

    tb.next()
    _zero_inputs(tb)
    tb.expect("num_valid", 2, msg="T4a: 2 entries after first batch")
    tb.expect("in_ready", 1, msg="T4a: still has space")

    # Second batch to fill to 4
    tb.drive("in_valid", 1)
    tb.drive("in_num", 2)
    tb.drive("in_inst_0", 0xA003)
    tb.drive("in_pc_0", 0x2008)
    tb.drive("in_inst_1", 0xA004)
    tb.drive("in_pc_1", 0x200C)

    tb.next()
    _zero_inputs(tb)
    tb.expect("num_valid", 4, msg="T4b: full (4 entries)")
    tb.expect("in_ready", 0, msg="T4b: backpressure when full")

    # ── T5: flush ────────────────────────────────────────────────
    tb.drive("flush", 1)

    tb.next()
    _zero_inputs(tb)
    tb.expect("num_valid", 0, msg="T5: flushed → empty")
    tb.expect("in_ready", 1, msg="T5: flushed → ready")
    for i in range(DEQ_W):
        tb.expect(f"out_valid_{i}", 0, msg=f"T5: out_valid_{i}=0")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_ibuffer_emit_mlir():
    mlir = compile_cycle_aware(
        build_ibuffer,
        name="ibuf",
        eager=True,
        size=SIZE,
        enq_width=ENQ_W,
        deq_width=DEQ_W,
        inst_width=INST_W,
        pc_width=PC_W,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "num_valid" in mlir
    assert "in_ready" in mlir


@pytest.mark.smoke
def test_ibuffer_default_emit_mlir():
    from top.parameters import (
        IBUFFER_SIZE,
        FETCH_BLOCK_SIZE,
        INST_BYTES,
        DECODE_WIDTH,
        PC_WIDTH,
    )

    mlir = compile_cycle_aware(
        build_ibuffer,
        name="ibuf_full",
        eager=True,
        size=IBUFFER_SIZE,
        enq_width=FETCH_BLOCK_SIZE // INST_BYTES,
        deq_width=DECODE_WIDTH,
        inst_width=32,
        pc_width=PC_WIDTH,
    ).emit_mlir()
    assert "func.func" in mlir


@pytest.mark.regcount
def test_ibuffer_has_entry_regs():
    import re

    mlir = compile_cycle_aware(
        build_ibuffer,
        name="ibuf_rc",
        eager=True,
        size=SIZE,
        enq_width=ENQ_W,
        deq_width=DEQ_W,
        inst_width=INST_W,
        pc_width=PC_W,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    min_regs = 2 + SIZE * 4  # 2 ptrs + 4 fields per entry (inst, pc, rvc, valid)
    assert n >= min_regs, f"IBuffer(sz={SIZE}) needs ≥{min_regs} regs; got {n}"


if __name__ == "__main__":
    test_ibuffer_emit_mlir()
    print("PASS: test_ibuffer_emit_mlir")
    test_ibuffer_default_emit_mlir()
    print("PASS: test_ibuffer_default_emit_mlir")
