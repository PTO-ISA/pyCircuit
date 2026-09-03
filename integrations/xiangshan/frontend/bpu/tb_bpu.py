"""Testbench for BPU / uBTB — MLIR smoke (L1) + functional directed (L2).

L2 scenarios test the uBTB (Micro Branch Target Buffer):
  T1  Cold start — prediction miss (no entries trained)
  T2  Train entry, then predict hit — correct target returned
  T3  Train different PC — verify isolation between entries

Uses tiny config: entries=4, pc_width=16, target_width=12.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from frontend.bpu.bpu import build_bpu  # noqa: E402
from frontend.bpu.ubtb import build_ubtb, ATTR_WIDTH  # noqa: E402

ENTRIES = 4
PC_W = 16
TAG_W = 8
TARGET_W = 12
USEFUL_W = 2
CFI_POS_W = 3
ATTR_W = 4  # BRANCH_TYPE_WIDTH + RAS_ACTION_WIDTH for small test


def _zero_inputs(tb: CycleAwareTb) -> None:
    tb.drive("s0_fire", 0)
    tb.drive("s0_pc", 0)
    tb.drive("enable", 1)
    tb.drive("train_valid", 0)
    tb.drive("train_pc", 0)
    tb.drive("train_target", 0)
    tb.drive("train_taken", 0)
    tb.drive("train_cfi_pos", 0)
    tb.drive("train_attr", 0)


@testbench
def tb_ubtb_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(40)

    # ── T1: Cold start — no entries, prediction should miss ──────
    _zero_inputs(tb)
    tb.drive("s0_fire", 1)
    tb.drive("s0_pc", 0x100)
    tb.expect("pred_valid", 0, msg="T1: cold miss — no entries trained")
    tb.expect("pred_taken", 0, msg="T1: pred_taken=0 on miss")

    # ── T2: Train with taken branch, then predict hit ────────────
    tb.next()
    _zero_inputs(tb)
    tb.drive("train_valid", 1)
    tb.drive("train_pc", 0x100)
    tb.drive("train_target", 0x200)
    tb.drive("train_taken", 1)
    tb.drive("train_cfi_pos", 2)
    tb.drive("train_attr", 5)

    # Let training write land
    tb.next()
    _zero_inputs(tb)

    # Now look up the same PC — should hit
    tb.drive("s0_fire", 1)
    tb.drive("s0_pc", 0x100)
    tb.expect("pred_valid", 1, msg="T2: hit after training")
    tb.expect("pred_taken", 1, msg="T2: pred_taken=1")
    tb.expect("pred_cfi_pos", 2, msg="T2: cfi_pos matches training")
    tb.expect("pred_attr", 5, msg="T2: attr matches training")

    # ── T3: Train different PC, verify distinct targets ──────────
    tb.next()
    _zero_inputs(tb)
    tb.drive("train_valid", 1)
    tb.drive("train_pc", 0x300)
    tb.drive("train_target", 0x400)
    tb.drive("train_taken", 1)
    tb.drive("train_cfi_pos", 1)
    tb.drive("train_attr", 3)

    tb.next()
    _zero_inputs(tb)

    # Look up second PC — should hit with its own target
    tb.drive("s0_fire", 1)
    tb.drive("s0_pc", 0x300)
    tb.expect("pred_valid", 1, msg="T3: second PC hits")
    tb.expect("pred_cfi_pos", 1, msg="T3: second PC cfi_pos")
    tb.expect("pred_attr", 3, msg="T3: second PC attr")

    # Verify first PC still hits correctly
    tb.next()
    _zero_inputs(tb)
    tb.drive("s0_fire", 1)
    tb.drive("s0_pc", 0x100)
    tb.expect("pred_valid", 1, msg="T3: first PC still hits")
    tb.expect("pred_cfi_pos", 2, msg="T3: first PC cfi_pos preserved")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_ubtb_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_ubtb,
        name="ubtb_s",
        eager=True,
        entries=ENTRIES,
        tag_width=TAG_W,
        target_width=TARGET_W,
        useful_cnt_width=USEFUL_W,
        pc_width=PC_W,
        cfi_pos_width=CFI_POS_W,
        attr_width=ATTR_W,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "pred_valid" in mlir
    assert "pred_target" in mlir


@pytest.mark.smoke
def test_bpu_emit_mlir():
    mlir = compile_cycle_aware(
        build_bpu,
        name="bpu",
        eager=True,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "@bpu" in mlir
    assert "pred_valid" in mlir


@pytest.mark.smoke
def test_ubtb_full_emit_mlir():
    from top.parameters import UBTB_NUM_ENTRIES

    mlir = compile_cycle_aware(
        build_ubtb,
        name="ubtb",
        eager=True,
        entries=UBTB_NUM_ENTRIES,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "@ubtb" in mlir


@pytest.mark.regcount
def test_ubtb_has_entry_regs():
    import re

    mlir = compile_cycle_aware(
        build_ubtb,
        name="ubtb_rc",
        eager=True,
        entries=ENTRIES,
        tag_width=TAG_W,
        target_width=TARGET_W,
        useful_cnt_width=USEFUL_W,
        pc_width=PC_W,
        cfi_pos_width=CFI_POS_W,
        attr_width=ATTR_W,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    # 6 fields per entry: valid, tag, target, cfi_pos, attr, useful
    min_regs = ENTRIES * 6
    assert n >= min_regs, f"uBTB(entries={ENTRIES}) needs ≥{min_regs} regs; got {n}"


if __name__ == "__main__":
    test_ubtb_small_emit_mlir()
    print("PASS: test_ubtb_small_emit_mlir")
    test_bpu_emit_mlir()
    print("PASS: test_bpu_emit_mlir")
    test_ubtb_full_emit_mlir()
    print("PASS: test_ubtb_full_emit_mlir")
    test_ubtb_has_entry_regs()
    print("PASS: test_ubtb_has_entry_regs")
