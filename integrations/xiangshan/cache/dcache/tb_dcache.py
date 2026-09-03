"""Testbench for DCache — MLIR smoke (L1) + functional directed (L2).

L2 tests cover cold-miss, refill-then-load-hit, store-hit, and
multi-set behaviour on the 4-stage pipeline.  Uses small config
(4 sets, 2 ways, 8B block, 16-bit PA) for fast compilation.
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

from cache.dcache.dcache import build_dcache  # noqa: E402
from top.parameters import DCACHE_SETS, DCACHE_WAYS, DCACHE_BLOCK_BYTES  # noqa: E402

N_SETS = 4
N_WAYS = 2
BLOCK_BYTES = 8
PADDR_W = 16
BLOCK_BITS = BLOCK_BYTES * 8
OFFSET_BITS = int(math.log2(BLOCK_BYTES))
INDEX_BITS = int(math.log2(N_SETS))
TAG_BITS = PADDR_W - INDEX_BITS - OFFSET_BITS
WAY_BITS = int(math.log2(N_WAYS))

BLOCK_MASK = (1 << BLOCK_BITS) - 1
FULL_WMASK = (1 << BLOCK_BYTES) - 1


def _make_addr(tag: int, set_idx: int) -> int:
    """Build a physical address from tag and set index (offset = 0)."""
    return (tag << (INDEX_BITS + OFFSET_BITS)) | (set_idx << OFFSET_BITS)


def _idle(tb: CycleAwareTb) -> None:
    """Deassert all request / control signals."""
    tb.drive("load_valid", 0)
    tb.drive("store_valid", 0)
    tb.drive("refill_valid", 0)
    tb.drive("flush", 0)


# ── L2: Functional testbench ────────────────────────────────────


@testbench
def tb_dcache_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(80)

    _idle(tb)
    tb.drive("load_vaddr", 0)
    tb.drive("load_ptag", 0)
    tb.drive("store_vaddr", 0)
    tb.drive("store_ptag", 0)
    tb.drive("store_wdata", 0)
    tb.drive("store_wmask", 0)
    tb.drive("refill_set", 0)
    tb.drive("refill_tag", 0)
    tb.drive("refill_way", 0)
    tb.drive("refill_data", 0)

    # ── T1: Load miss on cold cache ──────────────────────────────
    addr_a = _make_addr(tag=1, set_idx=0)
    tb.drive("load_valid", 1)
    tb.drive("load_vaddr", addr_a)
    tb.drive("load_ptag", 1)

    tb.next()  # s1: tag compare
    _idle(tb)
    tb.next()  # s2: miss decision
    tb.expect("miss_valid", 1, msg="T1: cold cache → miss")

    tb.next()  # s3: response
    tb.expect("load_resp_valid", 0, msg="T1: miss → no load resp")
    tb.expect("load_resp_hit", 0, msg="T1: miss → no hit")

    # ── T2: Refill → load hit ───────────────────────────────────
    refill_data_a = 0xDEAD_BEEF_CAFE_0001 & BLOCK_MASK
    tb.next()
    tb.drive("refill_valid", 1)
    tb.drive("refill_set", 0)
    tb.drive("refill_tag", 1)
    tb.drive("refill_way", 0)
    tb.drive("refill_data", refill_data_a)

    tb.next()
    _idle(tb)
    tb.drive("load_valid", 1)
    tb.drive("load_vaddr", addr_a)
    tb.drive("load_ptag", 1)

    tb.next()  # s1
    _idle(tb)
    tb.next()  # s2
    tb.next()  # s3
    tb.expect("load_resp_valid", 1, msg="T2: refilled → valid")
    tb.expect("load_resp_hit", 1, msg="T2: refilled → hit")
    tb.expect("load_resp_data", refill_data_a, msg="T2: correct data")

    # ── T3: Store hit (same refilled line) ───────────────────────
    store_data = 0x1122_3344_5566_7788 & BLOCK_MASK
    tb.next()
    tb.drive("store_valid", 1)
    tb.drive("store_vaddr", addr_a)
    tb.drive("store_ptag", 1)
    tb.drive("store_wdata", store_data)
    tb.drive("store_wmask", FULL_WMASK)

    tb.next()  # s1
    _idle(tb)
    tb.next()  # s2
    tb.next()  # s3
    tb.expect("store_resp_valid", 1, msg="T3: store → valid resp")
    tb.expect("store_resp_hit", 1, msg="T3: store → hit")

    # ── T4: Different set — independent ──────────────────────────
    addr_b = _make_addr(tag=1, set_idx=1)
    refill_data_b = 0xAAAA_BBBB_CCCC_DDDD & BLOCK_MASK
    tb.next()
    tb.drive("refill_valid", 1)
    tb.drive("refill_set", 1)
    tb.drive("refill_tag", 1)
    tb.drive("refill_way", 0)
    tb.drive("refill_data", refill_data_b)

    tb.next()
    _idle(tb)
    tb.drive("load_valid", 1)
    tb.drive("load_vaddr", addr_b)
    tb.drive("load_ptag", 1)

    tb.next()  # s1
    _idle(tb)
    tb.next()  # s2
    tb.next()  # s3
    tb.expect("load_resp_valid", 1, msg="T4: set-1 → valid")
    tb.expect("load_resp_hit", 1, msg="T4: set-1 → hit")
    tb.expect("load_resp_data", refill_data_b, msg="T4: set-1 data")

    tb.finish()


# ── L1: MLIR smoke (pytest) ─────────────────────────────────────


@pytest.mark.smoke
def test_dcache_emit_mlir():
    mlir = compile_cycle_aware(
        build_dcache,
        name="dcache",
        eager=True,
        n_sets=DCACHE_SETS,
        n_ways=DCACHE_WAYS,
        block_bytes=DCACHE_BLOCK_BYTES,
        paddr_width=36,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "@dcache" in mlir
    assert "load_resp_valid" in mlir


@pytest.mark.smoke
def test_dcache_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_dcache,
        name="dcache_s",
        eager=True,
        n_sets=N_SETS,
        n_ways=N_WAYS,
        block_bytes=BLOCK_BYTES,
        paddr_width=PADDR_W,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "load_resp_valid" in mlir


@pytest.mark.regcount
def test_dcache_has_state_regs():
    import re

    mlir = compile_cycle_aware(
        build_dcache,
        name="dcache_rc",
        eager=True,
        n_sets=N_SETS,
        n_ways=N_WAYS,
        block_bytes=BLOCK_BYTES,
        paddr_width=PADDR_W,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    assert n >= 10, f"DCache must have ≥10 state registers; got {n}"


if __name__ == "__main__":
    test_dcache_small_emit_mlir()
    print("PASS: test_dcache_small_emit_mlir")
    test_dcache_emit_mlir()
    print("PASS: test_dcache_emit_mlir")
    test_dcache_has_state_regs()
    print("PASS: test_dcache_has_state_regs")
