"""Testbench for FTQ — MLIR smoke (L1) + functional directed (L2).

L2 scenarios:
  T1  Empty after reset — bpu_in_ready=1, ifu_req_valid=0
  T2  BPU write → IFU read consistency
  T3  Redirect rolls back pointer
  T4  Commit frees space

Uses tiny config: size=4, pc_width=16, cfi_offset_width=3, bpu_run_ahead=2.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from frontend.ftq.ftq import build_ftq  # noqa: E402

FTQ_SZ = 4
PC_W = 16
CFI_OFF_W = 3
RUN_AHEAD = 2
IDX_W = 2  # log2(4)
PTR_W = 3  # idx + 1


def _zero_inputs(tb: CycleAwareTb) -> None:
    tb.drive("bpu_in_valid", 0)
    tb.drive("bpu_in_start_pc", 0)
    tb.drive("bpu_in_target", 0)
    tb.drive("bpu_in_taken", 0)
    tb.drive("bpu_in_cfi_offset", 0)
    tb.drive("bpu_s3_override", 0)
    tb.drive("bpu_s3_ptr", 0)
    tb.drive("ifu_req_ready", 0)
    tb.drive("ifu_wb_valid", 0)
    tb.drive("redirect_valid", 0)
    tb.drive("redirect_ftq_idx", 0)
    tb.drive("commit_valid", 0)


@testbench
def tb_ftq_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(50)

    # ── T1: Empty after reset ────────────────────────────────────
    _zero_inputs(tb)
    tb.expect("bpu_in_ready", 1, msg="T1: ready when empty")
    tb.expect("ifu_req_valid", 0, msg="T1: no fetch request when empty")
    tb.expect("bpu_ptr_out", 0, msg="T1: bpu_ptr=0")
    tb.expect("commit_ptr_out", 0, msg="T1: commit_ptr=0")

    # ── T2: BPU writes prediction → IFU reads it ────────────────
    tb.next()
    _zero_inputs(tb)
    tb.drive("bpu_in_valid", 1)
    tb.drive("bpu_in_start_pc", 0x1000)
    tb.drive("bpu_in_target", 0x2000)
    tb.drive("bpu_in_taken", 1)
    tb.drive("bpu_in_cfi_offset", 3)
    tb.expect("bpu_in_ready", 1, msg="T2: ready for write")

    # After clock: entry written, bpu_ptr advances
    tb.next()
    _zero_inputs(tb)
    tb.expect("bpu_ptr_out", 1, msg="T2: bpu_ptr advanced to 1")
    tb.expect("ifu_req_valid", 1, msg="T2: IFU sees entry")

    # IFU reads: drive ifu_req_ready to consume
    tb.drive("ifu_req_ready", 1)
    tb.expect("ifu_req_start_pc", 0x1000, msg="T2: IFU reads correct PC")
    tb.expect("ifu_req_target", 0x2000, msg="T2: IFU reads correct target")
    tb.expect("ifu_req_taken", 1, msg="T2: IFU reads taken=1")
    tb.expect("ifu_req_cfi_offset", 3, msg="T2: IFU reads correct cfi_offset")

    # Write second prediction
    tb.next()
    _zero_inputs(tb)
    tb.drive("bpu_in_valid", 1)
    tb.drive("bpu_in_start_pc", 0x2000)
    tb.drive("bpu_in_target", 0x3000)
    tb.drive("bpu_in_taken", 0)
    tb.drive("bpu_in_cfi_offset", 0)

    tb.next()
    _zero_inputs(tb)
    tb.drive("ifu_req_ready", 1)
    tb.expect("ifu_req_start_pc", 0x2000, msg="T2b: second entry PC")
    tb.expect("ifu_req_taken", 0, msg="T2b: second entry taken=0")

    # ── T3: Redirect rolls back pointer ──────────────────────────
    tb.next()
    _zero_inputs(tb)
    tb.drive("redirect_valid", 1)
    tb.drive("redirect_ftq_idx", 0)  # roll back to idx 0

    tb.next()
    _zero_inputs(tb)
    # bpu_ptr, ifu_ptr should be rolled back to redirect_ftq_idx + 1 = 1
    tb.expect("bpu_ptr_out", 1, msg="T3: bpu_ptr rolled back to 1")

    # ── T4: Commit frees space ───────────────────────────────────
    # First enqueue an entry
    tb.drive("bpu_in_valid", 1)
    tb.drive("bpu_in_start_pc", 0x5000)
    tb.drive("bpu_in_target", 0x6000)
    tb.drive("bpu_in_taken", 1)
    tb.drive("bpu_in_cfi_offset", 2)

    tb.next()
    _zero_inputs(tb)

    # Commit the entry at commit_ptr
    tb.drive("commit_valid", 1)

    tb.next()
    _zero_inputs(tb)
    tb.expect("commit_ptr_out", 1, msg="T4: commit_ptr advanced")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_ftq_small_emit_mlir():
    mlir = compile_cycle_aware(
        build_ftq,
        name="ftq_s",
        eager=True,
        size=FTQ_SZ,
        pc_width=PC_W,
        cfi_offset_width=CFI_OFF_W,
        bpu_run_ahead=RUN_AHEAD,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "bpu_in_ready" in mlir
    assert "ifu_req_valid" in mlir


@pytest.mark.smoke
def test_ftq_full_emit_mlir():
    from top.parameters import FTQ_SIZE, PC_WIDTH
    from frontend.ftq.ftq import CFI_OFFSET_WIDTH, BPU_RUN_AHEAD_DISTANCE

    mlir = compile_cycle_aware(
        build_ftq,
        name="ftq",
        eager=True,
        size=FTQ_SIZE,
        pc_width=PC_WIDTH,
        cfi_offset_width=CFI_OFFSET_WIDTH,
        bpu_run_ahead=BPU_RUN_AHEAD_DISTANCE,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "@ftq" in mlir


@pytest.mark.regcount
def test_ftq_has_entry_regs():
    import re

    mlir = compile_cycle_aware(
        build_ftq,
        name="ftq_rc",
        eager=True,
        size=FTQ_SZ,
        pc_width=PC_W,
        cfi_offset_width=CFI_OFF_W,
        bpu_run_ahead=RUN_AHEAD,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    # 4 ptrs + 5 fields per entry (pc, target, taken, cfi_off, valid)
    min_regs = 4 + FTQ_SZ * 5
    assert n >= min_regs, f"FTQ(sz={FTQ_SZ}) needs ≥{min_regs} regs; got {n}"


if __name__ == "__main__":
    test_ftq_small_emit_mlir()
    print("PASS: test_ftq_small_emit_mlir")
    test_ftq_full_emit_mlir()
    print("PASS: test_ftq_full_emit_mlir")
    test_ftq_has_entry_regs()
    print("PASS: test_ftq_has_entry_regs")
