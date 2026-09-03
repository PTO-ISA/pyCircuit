"""Testbench for Load Unit — MLIR smoke (L1) + functional directed (L2).

L2 scenarios:
  T1  Load pipeline stages: issue → TLB → DCache → writeback
  T2  Pipeline valid propagation with flush kill

Uses small config: data_width=16, addr_width=16.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from mem.pipeline.load_unit import build_load_unit  # noqa: E402

DATA_W = 16
ADDR_W = 16
ROB_IDX_W = 4
LQ_IDX_W = 4
PPN_W = 4  # small ppn for testing (paddr = ppn:4 ++ offset:12 = 16 bits)


def _zero_inputs(tb: CycleAwareTb) -> None:
    tb.drive("flush", 0)
    tb.drive("issue_valid", 0)
    tb.drive("issue_addr", 0)
    tb.drive("issue_rob_idx", 0)
    tb.drive("issue_lq_idx", 0)
    tb.drive("fwd_valid", 0)
    tb.drive("fwd_data", 0)
    tb.drive("tlb_resp_hit", 0)
    tb.drive("tlb_resp_ppn", 0)
    tb.drive("dcache_resp_valid", 0)
    tb.drive("dcache_resp_data", 0)


@testbench
def tb_load_unit_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(30)

    # ── T1: Load pipeline — issue → TLB hit → DCache → writeback ─
    _zero_inputs(tb)

    # s0: Issue load
    tb.drive("issue_valid", 1)
    tb.drive("issue_addr", 0x1ABC)  # vpn = 0x1, offset = 0xABC
    tb.drive("issue_rob_idx", 5)
    tb.drive("issue_lq_idx", 2)
    tb.expect("tlb_req_valid", 1, msg="T1-s0: TLB request fires")

    # s1: TLB responds with hit
    tb.next()
    _zero_inputs(tb)
    tb.drive("tlb_resp_hit", 1)
    tb.drive("tlb_resp_ppn", 0xD)  # paddr = 0xD:ABC = 0xDABC
    tb.expect("dcache_req_valid", 1, msg="T1-s1: DCache request fires")
    tb.expect("lq_update_valid", 1, msg="T1-s1: LQ update fires")

    # s2: DCache responds with data
    tb.next()
    _zero_inputs(tb)
    tb.drive("dcache_resp_valid", 1)
    tb.drive("dcache_resp_data", 0x42)

    # s2 output (after domain.next boundaries)
    tb.next()
    _zero_inputs(tb)
    tb.expect("wb_valid", 1, msg="T1-s2: writeback valid")
    tb.expect("wb_data", 0x42, msg="T1-s2: writeback data correct")
    tb.expect("wb_rob_idx", 5, msg="T1-s2: rob_idx preserved")
    tb.expect("wb_lq_idx", 2, msg="T1-s2: lq_idx preserved")

    # ── T2: Pipeline kill on flush ───────────────────────────────
    tb.next()
    _zero_inputs(tb)

    # Issue another load
    tb.drive("issue_valid", 1)
    tb.drive("issue_addr", 0x2000)
    tb.drive("issue_rob_idx", 7)
    tb.drive("issue_lq_idx", 3)

    tb.next()
    _zero_inputs(tb)
    # Assert flush in s1 — should kill the pipeline
    tb.drive("flush", 1)
    tb.drive("tlb_resp_hit", 1)
    tb.drive("tlb_resp_ppn", 0xE)
    # s1 alive = valid & ~flush = 0 when flushed
    tb.expect("dcache_req_valid", 0, msg="T2: flush kills s1 dcache req")

    tb.next()
    _zero_inputs(tb)
    # No writeback expected since pipeline was killed
    tb.next()
    _zero_inputs(tb)
    tb.expect("wb_valid", 0, msg="T2: no writeback after flush")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_load_unit_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_load_unit,
        name="lu_s",
        eager=True,
        data_width=DATA_W,
        addr_width=ADDR_W,
        rob_idx_width=ROB_IDX_W,
        lq_idx_width=LQ_IDX_W,
        ppn_width=PPN_W,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "wb_valid" in mlir
    assert "tlb_req_valid" in mlir
    assert "dcache_req_valid" in mlir


@pytest.mark.smoke
def test_load_unit_default_emit_mlir():
    mlir = compile_cycle_aware(
        build_load_unit,
        name="load_unit",
        eager=True,
    ).emit_mlir()
    assert "func.func" in mlir or "hw." in mlir
    assert "load_unit" in mlir


@pytest.mark.regcount
def test_load_unit_has_pipeline_regs():
    import re

    mlir = compile_cycle_aware(
        build_load_unit,
        name="lu_rc",
        eager=True,
        data_width=DATA_W,
        addr_width=ADDR_W,
        rob_idx_width=ROB_IDX_W,
        lq_idx_width=LQ_IDX_W,
        ppn_width=PPN_W,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    # s0→s1 regs: valid, addr, rob_idx, lq_idx (4)
    # s1→s2 regs: valid, rob_idx, lq_idx, paddr (4)
    min_regs = 8
    assert n >= min_regs, f"LoadUnit needs ≥{min_regs} pipeline regs; got {n}"


if __name__ == "__main__":
    test_load_unit_small_emit_mlir()
    print("PASS: test_load_unit_small_emit_mlir")
    test_load_unit_default_emit_mlir()
    print("PASS: test_load_unit_default_emit_mlir")
    test_load_unit_has_pipeline_regs()
    print("PASS: test_load_unit_has_pipeline_regs")
