"""Testbench for Dispatch — MLIR smoke (L1) + functional directed (L2).

L2 tests verify FU-type based routing to int/fp/mem issue queues,
backpressure stall propagation, and output passthrough correctness.
Uses dispatch_width=2, 4-bit ptag, 16-bit PC for fast compilation.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.dispatch.dispatch import (  # noqa: E402
    FU_ALU,
    FU_BRU,
    FU_FPU,
    FU_LDU,
    dispatch,
)

DP_W = 2
FU_W = 3
PTAG_W = 4
PC_W = 16
ROB_W = 4
P = "dp"


def _compile_dispatch(*, dispatch_width: int = DP_W, name: str = "dispatch"):
    return compile_cycle_aware(
        dispatch,
        name=name,
        eager=True,
        prefix=P,
        dispatch_width=dispatch_width,
        fu_type_width=FU_W,
        ptag_w=PTAG_W,
        pc_width=PC_W,
        rob_idx_w=ROB_W,
    )


def _drive_idle(tb: CycleAwareTb, *, width: int = DP_W) -> None:
    """Drive all inputs to zero (vector ports as lane lists, scalars as 0)."""
    zeros1 = [0] * width
    tb.drive("dp_in_valid", zeros1)
    tb.drive("dp_in_pdest", zeros1)
    tb.drive("dp_in_psrc1", zeros1)
    tb.drive("dp_in_psrc2", zeros1)
    tb.drive("dp_in_old_pdest", zeros1)
    tb.drive("dp_in_fu_type", zeros1)
    tb.drive("dp_in_rob_idx", zeros1)
    tb.drive("dp_in_pc", zeros1)
    tb.drive("dp_iq_int_ready", 1)
    tb.drive("dp_iq_fp_ready", 1)
    tb.drive("dp_iq_mem_ready", 1)
    tb.drive("dp_flush", 0)


@testbench
def tb_dispatch_functional(t: Tb) -> None:
    # Compile the device once and bind it so vector ports accept lane lists.
    circuit = _compile_dispatch(name="dispatch_tb")
    tb = CycleAwareTb(t, circuit=circuit)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(30)

    _drive_idle(tb)
    tb.expect("dp_stall", 0, msg="T1: no stall when idle")
    tb.expect("dp_iq_int_valid", [0, 0], msg="T1: int_valid=0")
    tb.expect("dp_iq_fp_valid", [0, 0], msg="T1: fp_valid=0")
    tb.expect("dp_iq_mem_valid", [0, 0], msg="T1: mem_valid=0")
    tb.expect("dp_rob_enq_valid", [0, 0], msg="T1: rob_enq=0")
    tb.expect("dp_int_dispatch_count", 0, msg="T1: int_cnt=0")
    tb.expect("dp_fp_dispatch_count", 0, msg="T1: fp_cnt=0")
    tb.expect("dp_mem_dispatch_count", 0, msg="T1: mem_cnt=0")

    # T2a: ALU (int) + LDU (mem)
    tb.next()
    _drive_idle(tb)
    tb.drive("dp_in_valid", [1, 1])
    tb.drive("dp_in_fu_type", [FU_ALU, FU_LDU])
    tb.drive("dp_in_pdest", [1, 4])
    tb.drive("dp_in_psrc1", [2, 0])
    tb.drive("dp_in_psrc2", [3, 0])
    tb.drive("dp_in_pc", [0x100, 0x104])
    tb.drive("dp_in_rob_idx", [0, 1])

    tb.expect("dp_stall", 0, msg="T2a: no stall")
    tb.expect("dp_iq_int_valid", [1, 0], msg="T2a: slot0→int")
    tb.expect("dp_iq_fp_valid", [0, 0], msg="T2a: no fp")
    tb.expect("dp_iq_mem_valid", [0, 1], msg="T2a: slot1→mem")
    tb.expect("dp_rob_enq_valid", [1, 1], msg="T2a: both rob")
    tb.expect("dp_out_pdest", [1, 4], msg="T2a: pdest")
    tb.expect("dp_out_psrc1", [2, 0], msg="T2a: psrc1")
    tb.expect("dp_out_pc", [0x100, 0x104], msg="T2a: pc")
    tb.expect("dp_int_dispatch_count", 1, msg="T2a: int_cnt=1")
    tb.expect("dp_fp_dispatch_count", 0, msg="T2a: fp_cnt=0")
    tb.expect("dp_mem_dispatch_count", 1, msg="T2a: mem_cnt=1")

    # T2b: FPU + BRU
    tb.next()
    _drive_idle(tb)
    tb.drive("dp_in_valid", [1, 1])
    tb.drive("dp_in_fu_type", [FU_FPU, FU_BRU])
    tb.drive("dp_in_pdest", [5, 6])

    tb.expect("dp_stall", 0, msg="T2b: no stall")
    tb.expect("dp_iq_fp_valid", [1, 0], msg="T2b: slot0→fp")
    tb.expect("dp_iq_int_valid", [0, 1], msg="T2b: slot1→int")
    tb.expect("dp_iq_mem_valid", [0, 0], msg="T2b: no mem")
    tb.expect("dp_int_dispatch_count", 1, msg="T2b: int_cnt=1")
    tb.expect("dp_fp_dispatch_count", 1, msg="T2b: fp_cnt=1")
    tb.expect("dp_mem_dispatch_count", 0, msg="T2b: mem_cnt=0")

    # T3a: int IQ full
    tb.next()
    _drive_idle(tb)
    tb.drive("dp_in_valid", [1, 0])
    tb.drive("dp_in_fu_type", [FU_ALU, 0])
    tb.drive("dp_iq_int_ready", 0)

    tb.expect("dp_stall", 1, msg="T3a: stall when int IQ full")
    tb.expect("dp_iq_int_valid", [0, 0], msg="T3a: no int dispatch")
    tb.expect("dp_rob_enq_valid", [0, 0], msg="T3a: no rob enq")

    # T3b: blocked slot stalls group
    tb.next()
    _drive_idle(tb)
    tb.drive("dp_in_valid", [1, 1])
    tb.drive("dp_in_fu_type", [FU_ALU, FU_LDU])
    tb.drive("dp_iq_mem_ready", 0)

    tb.expect("dp_stall", 1, msg="T3b: blocked slot1 stalls all")
    tb.expect("dp_iq_int_valid", [0, 0], msg="T3b: slot0 blocked too")
    tb.expect("dp_iq_mem_valid", [0, 0], msg="T3b: slot1 blocked")
    tb.expect("dp_rob_enq_valid", [0, 0], msg="T3b: no rob")
    tb.expect("dp_int_dispatch_count", 0, msg="T3b: int_cnt=0")
    tb.expect("dp_mem_dispatch_count", 0, msg="T3b: mem_cnt=0")

    # T3c: unblock
    tb.next()
    _drive_idle(tb)
    tb.drive("dp_in_valid", [1, 1])
    tb.drive("dp_in_fu_type", [FU_ALU, FU_LDU])

    tb.expect("dp_stall", 0, msg="T3c: no stall after unblock")
    tb.expect("dp_iq_int_valid", [1, 0], msg="T3c: slot0")
    tb.expect("dp_iq_mem_valid", [0, 1], msg="T3c: slot1")
    tb.expect("dp_rob_enq_valid", [1, 1], msg="T3c: both rob")

    tb.finish()


@pytest.mark.smoke
@pytest.mark.parametrize("dispatch_width", [1, 2, 4])
def test_dispatch_emit_mlir(dispatch_width: int):
    mlir = _compile_dispatch(
        dispatch_width=dispatch_width,
        name=f"dispatch_w{dispatch_width}",
    ).emit_mlir()
    assert "func.func" in mlir
    assert "stall" in mlir
    assert "vector<" in mlir
    assert "in_valid_0" not in mlir
    assert re.search(r"in_valid.*vector<", mlir) or "vector<" in mlir


@pytest.mark.smoke
def test_dispatch_small_emit_mlir():
    mlir = _compile_dispatch(name="dispatch_s").emit_mlir()
    assert "func.func" in mlir
    assert "stall" in mlir
    assert "iq_int_valid_0" not in mlir


@pytest.mark.regcount
def test_dispatch_has_pipeline_regs():
    mlir = _compile_dispatch(name="dispatch_rc").emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    assert n >= 1, f"Dispatch must keep a dp1 pipeline stage; got {n} pyc.reg"
    assert "vector<" in mlir


if __name__ == "__main__":
    test_dispatch_small_emit_mlir()
    print("PASS: test_dispatch_small_emit_mlir")
    for w in (1, 2, 4):
        test_dispatch_emit_mlir(w)
        print(f"PASS: test_dispatch_emit_mlir width={w}")
    test_dispatch_has_pipeline_regs()
    print("PASS: test_dispatch_has_pipeline_regs")
