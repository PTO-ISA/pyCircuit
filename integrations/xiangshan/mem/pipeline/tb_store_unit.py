"""Testbench for Store Unit — MLIR smoke (L1) + functional directed (L2).

L2 scenarios:
  T1  Store pipeline stages: issue → TLB → store queue write + writeback
  T2  Pipeline kill on flush

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

from mem.pipeline.store_unit import build_store_unit  # noqa: E402

DATA_W = 16
ADDR_W = 16
ROB_IDX_W = 4
SQ_IDX_W = 4
PPN_W = 4  # paddr = ppn:4 ++ offset:12 = 16 bits


def _zero_inputs(tb: CycleAwareTb) -> None:
    tb.drive("flush", 0)
    tb.drive("issue_valid", 0)
    tb.drive("issue_addr", 0)
    tb.drive("issue_data", 0)
    tb.drive("issue_rob_idx", 0)
    tb.drive("issue_sq_idx", 0)
    tb.drive("tlb_resp_hit", 0)
    tb.drive("tlb_resp_ppn", 0)


@testbench
def tb_store_unit_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(30)

    # ── T1: Store pipeline — issue → TLB hit → SQ write + WB ────
    _zero_inputs(tb)

    # s0: Issue store
    tb.drive("issue_valid", 1)
    tb.drive("issue_addr", 0x3ABC)  # vpn = 0x3, offset = 0xABC
    tb.drive("issue_data", 0xDEAD)
    tb.drive("issue_rob_idx", 9)
    tb.drive("issue_sq_idx", 1)
    tb.expect("tlb_req_valid", 1, msg="T1-s0: TLB request fires")

    # s1: TLB responds with hit → store queue write
    tb.next()
    _zero_inputs(tb)
    tb.drive("tlb_resp_hit", 1)
    tb.drive("tlb_resp_ppn", 0xF)  # paddr = 0xF:ABC = 0xFABC

    # After pipeline boundary: outputs ready
    tb.next()
    _zero_inputs(tb)
    tb.expect("sq_write_valid", 1, msg="T1-s1: SQ write fires")
    tb.expect("sq_write_idx", 1, msg="T1-s1: SQ write idx correct")
    tb.expect("sq_write_data", 0xDEAD, msg="T1-s1: SQ write data correct")
    tb.expect("wb_valid", 1, msg="T1-s1: writeback valid")
    tb.expect("wb_rob_idx", 9, msg="T1-s1: rob_idx preserved")

    # ── T2: Pipeline kill on flush ───────────────────────────────
    tb.next()
    _zero_inputs(tb)

    # Issue another store
    tb.drive("issue_valid", 1)
    tb.drive("issue_addr", 0x4000)
    tb.drive("issue_data", 0xCAFE)
    tb.drive("issue_rob_idx", 10)
    tb.drive("issue_sq_idx", 2)

    tb.next()
    _zero_inputs(tb)
    # Flush during s1 — kills the pipeline
    tb.drive("flush", 1)
    tb.drive("tlb_resp_hit", 1)
    tb.drive("tlb_resp_ppn", 0xA)

    tb.next()
    _zero_inputs(tb)
    tb.expect("sq_write_valid", 0, msg="T2: flush kills SQ write")
    tb.expect("wb_valid", 0, msg="T2: flush kills writeback")

    # ── T1b: TLB miss — no SQ write ─────────────────────────────
    tb.next()
    _zero_inputs(tb)

    tb.drive("issue_valid", 1)
    tb.drive("issue_addr", 0x5000)
    tb.drive("issue_data", 0x1234)
    tb.drive("issue_rob_idx", 11)
    tb.drive("issue_sq_idx", 3)

    tb.next()
    _zero_inputs(tb)
    tb.drive("tlb_resp_hit", 0)  # TLB miss
    tb.drive("tlb_resp_ppn", 0)
    tb.expect("tlb_miss", 1, msg="T1b: TLB miss signaled")

    tb.next()
    _zero_inputs(tb)
    tb.expect("sq_write_valid", 0, msg="T1b: no SQ write on TLB miss")
    tb.expect("wb_valid", 0, msg="T1b: no writeback on TLB miss")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_store_unit_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_store_unit,
        name="su_s",
        eager=True,
        data_width=DATA_W,
        addr_width=ADDR_W,
        rob_idx_width=ROB_IDX_W,
        sq_idx_width=SQ_IDX_W,
        ppn_width=PPN_W,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "sq_write_valid" in mlir
    assert "wb_valid" in mlir
    assert "tlb_req_valid" in mlir


@pytest.mark.smoke
def test_store_unit_default_emit_mlir():
    mlir = compile_cycle_aware(
        build_store_unit,
        name="store_unit",
        eager=True,
    ).emit_mlir()
    assert "func.func" in mlir or "hw." in mlir
    assert "store_unit" in mlir


@pytest.mark.regcount
def test_store_unit_has_pipeline_regs():
    import re

    mlir = compile_cycle_aware(
        build_store_unit,
        name="su_rc",
        eager=True,
        data_width=DATA_W,
        addr_width=ADDR_W,
        rob_idx_width=ROB_IDX_W,
        sq_idx_width=SQ_IDX_W,
        ppn_width=PPN_W,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    # s0→s1 regs: valid, addr, data, rob_idx, sq_idx (5)
    min_regs = 5
    assert n >= min_regs, f"StoreUnit needs ≥{min_regs} pipeline regs; got {n}"


if __name__ == "__main__":
    test_store_unit_small_emit_mlir()
    print("PASS: test_store_unit_small_emit_mlir")
    test_store_unit_default_emit_mlir()
    print("PASS: test_store_unit_default_emit_mlir")
    test_store_unit_has_pipeline_regs()
    print("PASS: test_store_unit_has_pipeline_regs")
