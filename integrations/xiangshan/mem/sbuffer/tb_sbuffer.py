"""Testbench for SBuffer — MLIR smoke (L1) + functional directed (L2).

L2 scenarios:
  T1  Write entry, then write same cache line — merge check
  T2  Drain to cache when threshold reached
  T3  Flush drains all valid entries

Uses tiny config: size=4, threshold=2, addr_width=16, data_width=16.
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from mem.sbuffer.sbuffer import build_sbuffer  # noqa: E402

SB_SZ = 4
THRESH = 2
ADDR_W = 16
DATA_W = 16
LINE_BYTES = 64
MASK_W = DATA_W // 8  # 2
LINE_BITS = int(math.log2(LINE_BYTES))  # 6


def _zero_inputs(tb: CycleAwareTb) -> None:
    tb.drive("flush", 0)
    tb.drive("enq_valid", 0)
    tb.drive("enq_addr", 0)
    tb.drive("enq_data", 0)
    tb.drive("enq_mask", 0)
    tb.drive("dcache_ready", 0)


@testbench
def tb_sbuffer_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(50)

    # ── T1: Write entry, then merge on same cache line ───────────
    _zero_inputs(tb)
    tb.expect("ready", 1, msg="T1: ready when empty")
    tb.expect("dcache_wr_valid", 0, msg="T1: no drain when empty")

    # Allocate entry for cache line at addr 0x1000 (tag = 0x1000>>6 = 0x40)
    tb.next()
    _zero_inputs(tb)
    tb.drive("enq_valid", 1)
    tb.drive("enq_addr", 0x1000)
    tb.drive("enq_data", 0x00AA)
    tb.drive("enq_mask", 0b01)

    tb.next()
    _zero_inputs(tb)
    tb.expect("ready", 1, msg="T1: still ready after one entry")

    # Merge: same cache line, different byte mask
    tb.drive("enq_valid", 1)
    tb.drive("enq_addr", 0x1000)
    tb.drive("enq_data", 0xBB00)
    tb.drive("enq_mask", 0b10)

    tb.next()
    _zero_inputs(tb)

    # Allocate new entry for different cache line
    tb.drive("enq_valid", 1)
    tb.drive("enq_addr", 0x2000)
    tb.drive("enq_data", 0xCCDD)
    tb.drive("enq_mask", 0b11)

    # ── T2: Drain when threshold reached ─────────────────────────
    tb.next()
    _zero_inputs(tb)
    # occupancy = 2 (merge didn't add new entry, so 1 + 1 = 2)
    # threshold = 2, so drain should trigger when dcache_ready
    tb.drive("dcache_ready", 1)
    tb.expect("dcache_wr_valid", 1, msg="T2: drain fires at threshold")

    tb.next()
    _zero_inputs(tb)
    tb.drive("dcache_ready", 1)
    # After draining one entry, occupancy drops

    # ── T3: Flush ────────────────────────────────────────────────
    tb.next()
    _zero_inputs(tb)
    tb.drive("flush", 1)
    tb.drive("dcache_ready", 1)

    tb.next()
    _zero_inputs(tb)
    tb.expect("dcache_wr_valid", 0, msg="T3: no drain after flush (all invalid)")
    tb.expect("ready", 1, msg="T3: ready after flush")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_sbuffer_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_sbuffer,
        name="sb_s",
        eager=True,
        size=SB_SZ,
        threshold=THRESH,
        addr_width=ADDR_W,
        data_width=DATA_W,
        line_bytes=LINE_BYTES,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "dcache_wr_valid" in mlir
    assert "ready" in mlir


@pytest.mark.smoke
def test_sbuffer_default_emit_mlir():
    mlir = compile_cycle_aware(
        build_sbuffer,
        name="sbuffer",
        eager=True,
        size=16,
        threshold=7,
        addr_width=36,
    ).emit_mlir()
    assert "func.func" in mlir or "hw." in mlir


@pytest.mark.regcount
def test_sbuffer_has_entry_regs():
    import re

    mlir = compile_cycle_aware(
        build_sbuffer,
        name="sb_rc",
        eager=True,
        size=SB_SZ,
        threshold=THRESH,
        addr_width=ADDR_W,
        data_width=DATA_W,
        line_bytes=LINE_BYTES,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    tag_w = ADDR_W - LINE_BITS
    # 1 occ counter + 4 fields per entry (valid, tag, data, mask)
    min_regs = 1 + SB_SZ * 4
    assert n >= min_regs, f"SBuffer(sz={SB_SZ}) needs ≥{min_regs} regs; got {n}"


if __name__ == "__main__":
    test_sbuffer_small_emit_mlir()
    print("PASS: test_sbuffer_small_emit_mlir")
    test_sbuffer_default_emit_mlir()
    print("PASS: test_sbuffer_default_emit_mlir")
    test_sbuffer_has_entry_regs()
    print("PASS: test_sbuffer_has_entry_regs")
