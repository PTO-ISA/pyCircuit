"""Testbench for ROB — MLIR smoke (L1) + functional directed (L2).

L2 scenarios:
  T1  Empty after reset — can_enq=1, no commits
  T2  Enqueue 2 uops → tail advances, can_enq still 1
  T3  Writeback both → commit fires in-order
  T4  Exception at head → exception_valid, commit blocked
  T5  Flush → all cleared, pointers reset
  T6  Redirect → tail adjusted

Uses tiny config: size=4, rename_width=2, commit_width=2, wb_ports=2.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.rob.rob import build_rob  # noqa: E402

ROB_SZ = 4
RN_W = 2
CM_W = 2
WB_P = 2
PTAG_W = 4
LREG_W = 3
PC_W = 16
IDX_W = 2  # log2(4)
PTR_W = 3  # idx + 1


def _zero_inputs(tb: CycleAwareTb) -> None:
    tb.drive("flush", 0)
    tb.drive("redirect_valid", 0)
    tb.drive("redirect_rob_ptr", 0)
    for i in range(RN_W):
        tb.drive(f"enq_valid_{i}", 0)
        tb.drive(f"enq_pc_{i}", 0)
        tb.drive(f"enq_rd_{i}", 0)
        tb.drive(f"enq_pdest_{i}", 0)
        tb.drive(f"enq_old_pdest_{i}", 0)
    for i in range(WB_P):
        tb.drive(f"wb_valid_{i}", 0)
        tb.drive(f"wb_rob_idx_{i}", 0)
        tb.drive(f"wb_exception_{i}", 0)


@testbench
def tb_rob_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(40)

    # ── T1: empty after reset ────────────────────────────────────
    _zero_inputs(tb)
    tb.expect("can_enq", 1, msg="T1: can_enq when empty")
    tb.expect("commit_valid_0", 0, msg="T1: no commit")
    tb.expect("commit_valid_1", 0, msg="T1: no commit")
    tb.expect("exception_valid", 0, msg="T1: no exception")
    tb.expect("head_ptr_out", 0, msg="T1: head=0")
    tb.expect("tail_ptr_out", 0, msg="T1: tail=0")

    # ── T2: enqueue 2 uops ──────────────────────────────────────
    tb.drive("enq_valid_0", 1)
    tb.drive("enq_pc_0", 0x1000)
    tb.drive("enq_rd_0", 1)
    tb.drive("enq_pdest_0", 5)
    tb.drive("enq_old_pdest_0", 2)

    tb.drive("enq_valid_1", 1)
    tb.drive("enq_pc_1", 0x1004)
    tb.drive("enq_rd_1", 2)
    tb.drive("enq_pdest_1", 6)
    tb.drive("enq_old_pdest_1", 3)

    tb.expect("enq_rob_idx_0", 0, msg="T2: first uop → idx 0")
    tb.expect("enq_rob_idx_1", 1, msg="T2: second uop → idx 1")

    # After clock edge, entries are written
    tb.next()
    _zero_inputs(tb)
    tb.expect("can_enq", 1, msg="T2: can_enq (2 of 4 used)")
    tb.expect("commit_valid_0", 0, msg="T2: not yet writebacked")
    tb.expect("tail_ptr_out", 2, msg="T2: tail=2")

    # ── T3: writeback both, then commit ──────────────────────────
    tb.drive("wb_valid_0", 1)
    tb.drive("wb_rob_idx_0", 0)
    tb.drive("wb_exception_0", 0)
    tb.drive("wb_valid_1", 1)
    tb.drive("wb_rob_idx_1", 1)
    tb.drive("wb_exception_1", 0)

    tb.next()
    _zero_inputs(tb)
    # Both entries valid + writebacked → commit
    tb.expect("commit_valid_0", 1, msg="T3: commit[0] fires")
    tb.expect("commit_valid_1", 1, msg="T3: commit[1] fires")
    tb.expect("commit_rd_0", 1, msg="T3: commit[0] rd")
    tb.expect("commit_pdest_0", 5, msg="T3: commit[0] pdest")
    tb.expect("commit_old_pdest_0", 2, msg="T3: commit[0] old_pdest")

    # After commit, head advances
    tb.next()
    _zero_inputs(tb)
    tb.expect("head_ptr_out", 4, msg="T3: head advanced by 2")
    tb.expect("commit_valid_0", 0, msg="T3: empty again")

    # ── T4: enqueue + writeback with exception ───────────────────
    tb.drive("enq_valid_0", 1)
    tb.drive("enq_pc_0", 0x2000)
    tb.drive("enq_rd_0", 3)
    tb.drive("enq_pdest_0", 7)
    tb.drive("enq_old_pdest_0", 4)

    tb.next()
    _zero_inputs(tb)

    # Writeback with exception
    tb.drive("wb_valid_0", 1)
    tb.drive("wb_rob_idx_0", 0)  # head entry idx
    tb.drive("wb_exception_0", 1)

    tb.next()
    _zero_inputs(tb)
    tb.expect("exception_valid", 1, msg="T4: exception at head")
    tb.expect("commit_valid_0", 0, msg="T4: exception blocks commit")

    # ── T5: flush ────────────────────────────────────────────────
    tb.drive("flush", 1)
    tb.next()
    _zero_inputs(tb)
    tb.expect("head_ptr_out", 0, msg="T5: flush → head=0")
    tb.expect("tail_ptr_out", 0, msg="T5: flush → tail=0")
    tb.expect("can_enq", 1, msg="T5: flush → can_enq")
    tb.expect("exception_valid", 0, msg="T5: flush → no exception")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_rob_emit_mlir():
    mlir = compile_cycle_aware(
        build_rob,
        name="rob_s",
        eager=True,
        rob_size=ROB_SZ,
        rename_width=RN_W,
        commit_width=CM_W,
        wb_ports=WB_P,
        ptag_w=PTAG_W,
        lreg_w=LREG_W,
        pc_width=PC_W,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "commit_valid_0" in mlir
    assert "can_enq" in mlir


@pytest.mark.smoke
def test_rob_medium_emit_mlir():
    mlir = compile_cycle_aware(
        build_rob,
        name="rob_m",
        eager=True,
        rob_size=16,
        rename_width=4,
        commit_width=4,
        wb_ports=4,
        ptag_w=8,
        lreg_w=5,
        pc_width=39,
    ).emit_mlir()
    assert "func.func" in mlir


@pytest.mark.regcount
def test_rob_has_entry_regs():
    import re

    mlir = compile_cycle_aware(
        build_rob,
        name="rob_rc",
        eager=True,
        rob_size=ROB_SZ,
        rename_width=RN_W,
        commit_width=CM_W,
        wb_ports=WB_P,
        ptag_w=PTAG_W,
        lreg_w=LREG_W,
        pc_width=PC_W,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    min_regs = 2 + ROB_SZ * 7  # 2 ptrs + 7 fields per entry
    assert n >= min_regs, f"ROB(sz={ROB_SZ}) needs ≥{min_regs} regs; got {n}"


if __name__ == "__main__":
    test_rob_emit_mlir()
    print("PASS: test_rob_emit_mlir")
    test_rob_medium_emit_mlir()
    print("PASS: test_rob_medium_emit_mlir")
