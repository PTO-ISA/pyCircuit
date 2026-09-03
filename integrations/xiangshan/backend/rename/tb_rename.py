"""Testbench for Rename — MLIR smoke (L1) + functional directed (L2).

L2 tests verify the RAT identity mapping at reset, free-list allocation,
intra-group WAW bypass, and snapshot-based redirect recovery.
Uses rename_width=2, int_phys_regs=8, int_logic_regs=5 for speed.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.rename.rename import build_rename  # noqa: E402

TEST_RW = 2
TEST_PREGS = 8
TEST_LREGS = 5
TEST_CW = 2
TEST_SNAPS = 2


@testbench
def tb_rename_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(40)

    # ── Safe defaults for all inputs ─────────────────────────────
    tb.drive("flush", 0)
    tb.drive("stall", 0)
    tb.drive("redirect_valid", 0)
    tb.drive("redirect_snap_id", 0)
    for i in range(TEST_RW):
        tb.drive(f"in_valid_{i}", 0)
        tb.drive(f"in_rd_{i}", 0)
        tb.drive(f"in_rs1_{i}", 0)
        tb.drive(f"in_rs2_{i}", 0)
        tb.drive(f"in_rd_valid_{i}", 0)
        tb.drive(f"in_rs1_valid_{i}", 0)
        tb.drive(f"in_rs2_valid_{i}", 0)
    for i in range(TEST_CW):
        tb.drive(f"commit_valid_{i}", 0)
        tb.drive(f"commit_old_pdest_{i}", 0)
        tb.drive(f"commit_rd_valid_{i}", 0)

    # ── Test 1: Identity mapping — RAT[i]=pi at reset ───────────
    # Read rat[1] and rat[2] via source operand lookup (no dest alloc)
    tb.drive("in_valid_0", 1)
    tb.drive("in_rd_0", 0)
    tb.drive("in_rs1_0", 1)
    tb.drive("in_rs2_0", 2)
    tb.drive("in_rd_valid_0", 0)
    tb.drive("in_rs1_valid_0", 1)
    tb.drive("in_rs2_valid_0", 1)

    tb.expect("out_valid_0", 1, msg="T1: valid")
    tb.expect("out_psrc1_0", 1, msg="T1: rat[1]=p1 (identity)")
    tb.expect("out_psrc2_0", 2, msg="T1: rat[2]=p2 (identity)")
    tb.expect("out_pdest_0", 0, msg="T1: no dest (x0, rd_valid=0)")
    tb.expect("can_alloc", 1, msg="T1: free regs available")

    # ── Test 2: Rename — pdest allocated from freelist ───────────
    # Initial FL: [p5, p6, p7], head=0, tail=3
    tb.next()
    tb.drive("in_valid_0", 1)
    tb.drive("in_rd_0", 1)
    tb.drive("in_rs1_0", 2)
    tb.drive("in_rs2_0", 3)
    tb.drive("in_rd_valid_0", 1)
    tb.drive("in_rs1_valid_0", 1)
    tb.drive("in_rs2_valid_0", 1)
    tb.drive("in_valid_1", 0)

    tb.expect("out_valid_0", 1, msg="T2: valid")
    tb.expect("out_pdest_0", 5, msg="T2: pdest=p5 (first free)")
    tb.expect("out_psrc1_0", 2, msg="T2: rat[2]=p2")
    tb.expect("out_psrc2_0", 3, msg="T2: rat[3]=p3")
    tb.expect("out_old_pdest_0", 1, msg="T2: old rat[1]=p1")

    # ── Test 3: WAW bypass within same group ─────────────────────
    # After T2: RAT=[0,5,2,3,4], FL head=1
    tb.next()
    tb.drive("in_valid_0", 1)
    tb.drive("in_rd_0", 2)
    tb.drive("in_rs1_0", 1)
    tb.drive("in_rs2_0", 3)
    tb.drive("in_rd_valid_0", 1)
    tb.drive("in_rs1_valid_0", 1)
    tb.drive("in_rs2_valid_0", 1)

    tb.drive("in_valid_1", 1)
    tb.drive("in_rd_1", 2)
    tb.drive("in_rs1_1", 2)
    tb.drive("in_rs2_1", 4)
    tb.drive("in_rd_valid_1", 1)
    tb.drive("in_rs1_valid_1", 1)
    tb.drive("in_rs2_valid_1", 1)

    # Slot 0: standard rename for x2
    tb.expect("out_pdest_0", 6, msg="T3: slot0 pdest=p6")
    tb.expect("out_psrc1_0", 5, msg="T3: slot0 rat[1]=p5 (from T2)")
    tb.expect("out_old_pdest_0", 2, msg="T3: slot0 old rat[2]=p2")

    # Slot 1: same rd=x2 → intra-group WAW bypass from slot 0
    tb.expect("out_pdest_1", 7, msg="T3: slot1 pdest=p7")
    tb.expect("out_psrc1_1", 6, msg="T3: slot1 psrc1 bypassed from slot0")
    tb.expect("out_psrc2_1", 4, msg="T3: slot1 rat[4]=p4")
    tb.expect("out_old_pdest_1", 6, msg="T3: slot1 old_pdest WAW bypass")

    # ── Test 4: Redirect restores snapshot ───────────────────────
    # After T3: RAT=[0,5,7,3,4], FL head=3
    # Snapshots taken per cycle when rename_fire:
    #   T1 → snap[0]={RAT=[0,1,2,3,4], fl_head=0}  (snap_next was 0)
    #   T2 → snap[1]={RAT=[0,1,2,3,4], fl_head=0}  (snap_next was 1)
    #   T3 → snap[0]={RAT=[0,5,2,3,4], fl_head=1}  (snap_next wrapped to 0)
    # Redirect to snap_id=1 restores pre-T2 identity mapping.
    tb.next()
    tb.drive("in_valid_0", 0)
    tb.drive("in_valid_1", 0)
    tb.drive("redirect_valid", 1)
    tb.drive("redirect_snap_id", 1)

    tb.next()
    tb.drive("redirect_valid", 0)

    # Verify RAT restored to identity: rat[1]=p1, rat[2]=p2
    tb.drive("in_valid_0", 1)
    tb.drive("in_rd_0", 0)
    tb.drive("in_rs1_0", 1)
    tb.drive("in_rs2_0", 2)
    tb.drive("in_rd_valid_0", 0)
    tb.drive("in_rs1_valid_0", 1)
    tb.drive("in_rs2_valid_0", 1)

    tb.expect("out_psrc1_0", 1, msg="T4: rat[1] restored to p1")
    tb.expect("out_psrc2_0", 2, msg="T4: rat[2] restored to p2")
    tb.expect("can_alloc", 1, msg="T4: FL head restored, regs available")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_rename_emit_mlir():
    from top.parameters import (
        COMMIT_WIDTH,
        INT_LOGIC_REGS,
        INT_PHYS_REGS,
        RENAME_SNAPSHOT_NUM,
        RENAME_WIDTH,
    )

    mlir = compile_cycle_aware(
        build_rename,
        name="rename",
        eager=True,
        rename_width=RENAME_WIDTH,
        int_phys_regs=INT_PHYS_REGS,
        int_logic_regs=INT_LOGIC_REGS,
        commit_width=COMMIT_WIDTH,
        snapshot_num=RENAME_SNAPSHOT_NUM,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "@rename" in mlir
    assert "out_pdest_0" in mlir


@pytest.mark.smoke
def test_rename_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_rename,
        name="rename_s",
        eager=True,
        rename_width=TEST_RW,
        int_phys_regs=TEST_PREGS,
        int_logic_regs=TEST_LREGS,
        commit_width=TEST_CW,
        snapshot_num=TEST_SNAPS,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "out_pdest_0" in mlir


@pytest.mark.regcount
def test_rename_has_state_regs():
    import re

    mlir = compile_cycle_aware(
        build_rename,
        name="rename_rc",
        eager=True,
        rename_width=TEST_RW,
        int_phys_regs=TEST_PREGS,
        int_logic_regs=TEST_LREGS,
        commit_width=TEST_CW,
        snapshot_num=TEST_SNAPS,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    assert n >= 20, f"Rename must have ≥20 state registers (RAT+FL+snaps); got {n}"


if __name__ == "__main__":
    test_rename_small_emit_mlir()
    print("PASS: test_rename_small_emit_mlir")
    test_rename_emit_mlir()
    print("PASS: test_rename_emit_mlir")
    test_rename_has_state_regs()
    print("PASS: test_rename_has_state_regs")
