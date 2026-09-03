"""Testbench for TLB — MLIR smoke (L1) + functional directed (L2).

L2 tests verify the 2-cycle pipeline with cold misses, refill-then-hit,
global sfence flush, and ASID-selective flush.  Uses small config
(4 entries, 16-bit VPN/PPN, 4-bit ASID) for fast compilation.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from cache.mmu.tlb import build_tlb  # noqa: E402
from top.parameters import ITLB_WAYS, ASID_LENGTH  # noqa: E402

TEST_WAYS = 4
VPN_W = 16
PPN_W = 16
ASID_W = 4


def _idle(tb: CycleAwareTb) -> None:
    """Deassert all request / control signals."""
    tb.drive("lookup_valid", 0)
    tb.drive("refill_valid", 0)
    tb.drive("flush", 0)
    tb.drive("flush_asid_valid", 0)


# ── L2: Functional testbench ────────────────────────────────────


@testbench
def tb_tlb_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(60)

    _idle(tb)
    tb.drive("lookup_vpn", 0)
    tb.drive("lookup_asid", 0)
    tb.drive("refill_vpn", 0)
    tb.drive("refill_ppn", 0)
    tb.drive("refill_asid", 0)
    tb.drive("flush_asid", 0)

    # ── T1: Cold start — lookup misses ───────────────────────────
    tb.drive("lookup_valid", 1)
    tb.drive("lookup_vpn", 0x1234)
    tb.drive("lookup_asid", 1)

    tb.next()
    _idle(tb)
    tb.expect("resp_valid", 1, msg="T1: resp valid after cold lookup")
    tb.expect("resp_hit", 0, msg="T1: cold → no hit")
    tb.expect("resp_miss", 1, msg="T1: cold → miss")
    tb.expect("ptw_req_valid", 1, msg="T1: PTW request on miss")

    # ── T2: Refill entry, then lookup hits ───────────────────────
    tb.next()
    tb.drive("refill_valid", 1)
    tb.drive("refill_vpn", 0x1234)
    tb.drive("refill_ppn", 0xABCD)
    tb.drive("refill_asid", 1)

    tb.next()
    _idle(tb)
    tb.drive("lookup_valid", 1)
    tb.drive("lookup_vpn", 0x1234)
    tb.drive("lookup_asid", 1)

    tb.next()
    _idle(tb)
    tb.expect("resp_valid", 1, msg="T2: resp valid")
    tb.expect("resp_hit", 1, msg="T2: refilled → hit")
    tb.expect("resp_ppn", 0xABCD, msg="T2: correct PPN")

    # ── T3: Global sfence flush → back to miss ──────────────────
    tb.next()
    tb.drive("flush", 1)

    tb.next()
    _idle(tb)
    tb.drive("lookup_valid", 1)
    tb.drive("lookup_vpn", 0x1234)
    tb.drive("lookup_asid", 1)

    tb.next()
    _idle(tb)
    tb.expect("resp_valid", 1, msg="T3: resp valid after flush")
    tb.expect("resp_hit", 0, msg="T3: flushed → miss")
    tb.expect("resp_miss", 1, msg="T3: flushed → miss")

    # ── T4: ASID-selective flush ─────────────────────────────────
    tb.next()
    tb.drive("refill_valid", 1)
    tb.drive("refill_vpn", 0x1111)
    tb.drive("refill_ppn", 0xAAAA)
    tb.drive("refill_asid", 1)

    tb.next()
    tb.drive("refill_vpn", 0x2222)
    tb.drive("refill_ppn", 0xBBBB)
    tb.drive("refill_asid", 2)

    tb.next()
    tb.drive("refill_valid", 0)
    tb.drive("flush_asid_valid", 1)
    tb.drive("flush_asid", 1)

    tb.next()
    _idle(tb)
    tb.drive("lookup_valid", 1)
    tb.drive("lookup_vpn", 0x1111)
    tb.drive("lookup_asid", 1)

    tb.next()
    _idle(tb)
    tb.expect("resp_valid", 1, msg="T4a: resp valid")
    tb.expect("resp_hit", 0, msg="T4a: ASID-1 flushed → miss")
    tb.expect("resp_miss", 1, msg="T4a: ASID-1 → miss")

    tb.next()
    tb.drive("lookup_valid", 1)
    tb.drive("lookup_vpn", 0x2222)
    tb.drive("lookup_asid", 2)

    tb.next()
    _idle(tb)
    tb.expect("resp_valid", 1, msg="T4b: resp valid")
    tb.expect("resp_hit", 1, msg="T4b: ASID-2 not flushed → hit")
    tb.expect("resp_ppn", 0xBBBB, msg="T4b: correct PPN")

    tb.finish()


# ── L1: MLIR smoke (pytest) ─────────────────────────────────────


@pytest.mark.smoke
def test_tlb_emit_mlir():
    mlir = compile_cycle_aware(
        build_tlb,
        name="tlb",
        eager=True,
        n_ways=ITLB_WAYS,
        vpn_width=27,
        ppn_width=24,
        asid_width=ASID_LENGTH,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "@tlb" in mlir
    assert "resp_hit" in mlir


@pytest.mark.smoke
def test_tlb_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_tlb,
        name="tlb_s",
        eager=True,
        n_ways=TEST_WAYS,
        vpn_width=VPN_W,
        ppn_width=PPN_W,
        asid_width=ASID_W,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "resp_hit" in mlir


@pytest.mark.regcount
def test_tlb_has_state_regs():
    import re

    mlir = compile_cycle_aware(
        build_tlb,
        name="tlb_rc",
        eager=True,
        n_ways=TEST_WAYS,
        vpn_width=VPN_W,
        ppn_width=PPN_W,
        asid_width=ASID_W,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    assert n >= 10, f"TLB (4 ways) must have ≥10 registers; got {n}"


if __name__ == "__main__":
    test_tlb_small_emit_mlir()
    print("PASS: test_tlb_small_emit_mlir")
    test_tlb_emit_mlir()
    print("PASS: test_tlb_emit_mlir")
    test_tlb_has_state_regs()
    print("PASS: test_tlb_has_state_regs")
