"""Testbench for ICache — MLIR smoke (L1) + functional directed (L2).

L2 tests cover cold fetch miss, refill-then-fetch-hit, and flush
behaviour on the 4-stage pipeline.  Uses small config (4 sets, 2 ways,
8B block, 16-bit PC) for fast compilation.
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

from frontend.icache.icache import build_icache  # noqa: E402
from top.parameters import (  # noqa: E402
    ICACHE_SETS,
    ICACHE_WAYS,
    ICACHE_BLOCK_BYTES,
    PC_WIDTH,
)

N_SETS = 4
N_WAYS = 2
BLOCK_BYTES = 8
PC_W = 16
BLOCK_BITS = BLOCK_BYTES * 8
OFFSET_BITS = int(math.log2(BLOCK_BYTES))
INDEX_BITS = int(math.log2(N_SETS))
TAG_BITS = PC_W - INDEX_BITS - OFFSET_BITS
WAY_BITS = int(math.log2(N_WAYS))

BLOCK_MASK = (1 << BLOCK_BITS) - 1


def _make_addr(tag: int, set_idx: int) -> int:
    """Build a virtual address from tag and set index (offset = 0)."""
    return (tag << (INDEX_BITS + OFFSET_BITS)) | (set_idx << OFFSET_BITS)


def _idle(tb: CycleAwareTb) -> None:
    """Deassert all request / control signals."""
    tb.drive("fetch_valid", 0)
    tb.drive("refill_valid", 0)
    tb.drive("flush", 0)


# ── L2: Functional testbench ────────────────────────────────────


@testbench
def tb_icache_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(60)

    _idle(tb)
    tb.drive("fetch_vaddr", 0)
    tb.drive("fetch_ptag", 0)
    tb.drive("refill_set", 0)
    tb.drive("refill_tag", 0)
    tb.drive("refill_way", 0)
    tb.drive("refill_data", 0)

    # ── T1: Miss on cold cache ───────────────────────────────────
    addr_a = _make_addr(tag=1, set_idx=0)
    tb.drive("fetch_valid", 1)
    tb.drive("fetch_vaddr", addr_a)
    tb.drive("fetch_ptag", 1)

    tb.next()  # s1
    _idle(tb)
    tb.next()  # s2: miss decision
    tb.expect("miss_valid", 1, msg="T1: cold cache → miss")

    tb.next()  # s3
    tb.expect("resp_valid", 0, msg="T1: miss → no resp")
    tb.expect("resp_hit", 0, msg="T1: miss → no hit")

    # ── T2: Refill → hit ────────────────────────────────────────
    refill_data = 0x0011_2233_4455_6677 & BLOCK_MASK
    tb.next()
    tb.drive("refill_valid", 1)
    tb.drive("refill_set", 0)
    tb.drive("refill_tag", 1)
    tb.drive("refill_way", 0)
    tb.drive("refill_data", refill_data)

    tb.next()
    _idle(tb)
    tb.drive("fetch_valid", 1)
    tb.drive("fetch_vaddr", addr_a)
    tb.drive("fetch_ptag", 1)

    tb.next()  # s1
    _idle(tb)
    tb.next()  # s2
    tb.next()  # s3
    tb.expect("resp_valid", 1, msg="T2: refilled → valid resp")
    tb.expect("resp_hit", 1, msg="T2: refilled → hit")
    tb.expect("resp_data", refill_data, msg="T2: correct instruction data")

    # ── T3: Flush → cached lines retained (simplified model) ────
    # ICache flush clears MSHR but does NOT invalidate cache lines.
    tb.next()
    tb.drive("flush", 1)
    tb.next()
    _idle(tb)

    tb.drive("fetch_valid", 1)
    tb.drive("fetch_vaddr", addr_a)
    tb.drive("fetch_ptag", 1)

    tb.next()  # s1
    _idle(tb)
    tb.next()  # s2
    tb.next()  # s3
    tb.expect("resp_valid", 1, msg="T3: cached line survives flush")
    tb.expect("resp_hit", 1, msg="T3: addr_a retained → hit")

    tb.finish()


# ── L1: MLIR smoke (pytest) ─────────────────────────────────────


@pytest.mark.smoke
def test_icache_emit_mlir():
    mlir = compile_cycle_aware(
        build_icache,
        name="icache",
        eager=True,
        n_sets=ICACHE_SETS,
        n_ways=ICACHE_WAYS,
        block_bytes=ICACHE_BLOCK_BYTES,
        pc_width=PC_WIDTH,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "@icache" in mlir
    assert "resp_valid" in mlir


@pytest.mark.smoke
def test_icache_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_icache,
        name="icache_s",
        eager=True,
        n_sets=N_SETS,
        n_ways=N_WAYS,
        block_bytes=BLOCK_BYTES,
        pc_width=PC_W,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "resp_valid" in mlir


@pytest.mark.regcount
def test_icache_has_state_regs():
    import re

    mlir = compile_cycle_aware(
        build_icache,
        name="icache_rc",
        eager=True,
        n_sets=N_SETS,
        n_ways=N_WAYS,
        block_bytes=BLOCK_BYTES,
        pc_width=PC_W,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    assert n >= 5, f"ICache must have ≥5 state registers; got {n}"


if __name__ == "__main__":
    test_icache_small_emit_mlir()
    print("PASS: test_icache_small_emit_mlir")
    test_icache_emit_mlir()
    print("PASS: test_icache_emit_mlir")
    test_icache_has_state_regs()
    print("PASS: test_icache_has_state_regs")
