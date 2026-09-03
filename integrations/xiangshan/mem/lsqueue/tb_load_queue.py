"""Testbench for Load Queue — MLIR smoke (L1) + functional directed (L2).

Uses tiny power-of-two config: size=4, addr_width=16.
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

from mem.lsqueue.load_queue import load_queue  # noqa: E402

LQ_SZ = 4
ADDR_W = 16
DATA_W = 16
ROB_IDX_W = 4
P = "ldq"


def _compile_lq(*, size: int = LQ_SZ, name: str = "lq_s"):
    return compile_cycle_aware(
        load_queue,
        name=name,
        eager=True,
        prefix=P,
        size=size,
        addr_width=ADDR_W,
        data_width=DATA_W,
        rob_idx_width=ROB_IDX_W,
    )


def _zero_inputs(tb: CycleAwareTb) -> None:
    tb.drive("ldq_flush", 0)
    tb.drive("ldq_enq_valid", 0)
    tb.drive("ldq_enq_rob_idx", 0)
    tb.drive("ldq_addr_update_valid", 0)
    tb.drive("ldq_addr_update_idx", 0)
    tb.drive("ldq_addr_update_addr", 0)
    tb.drive("ldq_commit_valid", 0)
    tb.drive("ldq_lookup_valid", 0)
    tb.drive("ldq_lookup_addr", 0)
    tb.drive("ldq_redirect_valid", 0)
    tb.drive("ldq_redirect_rob_idx", 0)


@testbench
def tb_load_queue_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(40)

    _zero_inputs(tb)
    tb.expect("ldq_count", 0, msg="T1: empty after reset")
    tb.expect("ldq_can_enqueue", 0, msg="T1: can_enqueue=0 (enq_valid=0)")

    tb.next()
    _zero_inputs(tb)
    tb.drive("ldq_enq_valid", 1)
    tb.drive("ldq_enq_rob_idx", 5)
    tb.expect("ldq_can_enqueue", 1, msg="T1: can enqueue when valid + space")
    tb.expect("ldq_enq_idx", 0, msg="T1: enqueue at idx 0")

    tb.next()
    _zero_inputs(tb)
    tb.expect("ldq_count", 1, msg="T1: count=1 after enqueue")

    tb.drive("ldq_enq_valid", 1)
    tb.drive("ldq_enq_rob_idx", 6)

    tb.next()
    _zero_inputs(tb)
    tb.expect("ldq_count", 2, msg="T1: count=2 after second enqueue")

    tb.drive("ldq_addr_update_valid", 1)
    tb.drive("ldq_addr_update_idx", 0)
    tb.drive("ldq_addr_update_addr", 0x1000)

    tb.next()
    _zero_inputs(tb)

    tb.drive("ldq_lookup_valid", 1)
    tb.drive("ldq_lookup_addr", 0x1000)
    tb.expect("ldq_violation_found", 1, msg="T2: violation found (same line)")

    tb.next()
    _zero_inputs(tb)

    tb.drive("ldq_lookup_valid", 1)
    tb.drive("ldq_lookup_addr", 0x2000)
    tb.expect("ldq_violation_found", 0, msg="T2: no violation (different line)")

    tb.next()
    _zero_inputs(tb)
    tb.drive("ldq_commit_valid", 1)

    tb.next()
    _zero_inputs(tb)
    tb.expect("ldq_count", 1, msg="T3: count=1 after commit (dequeued head)")

    tb.drive("ldq_commit_valid", 1)

    tb.next()
    _zero_inputs(tb)
    tb.expect("ldq_count", 0, msg="T3: count=0 after second commit")

    tb.finish()


@pytest.mark.smoke
@pytest.mark.parametrize("size", [4, 8])
def test_load_queue_small_emit_mlir(size: int):
    mlir = _compile_lq(size=size, name=f"lq_s{size}").emit_mlir()
    assert "func.func" in mlir
    assert "can_enqueue" in mlir
    assert "violation_found" in mlir
    assert "vector<" in mlir


@pytest.mark.regcount
def test_load_queue_has_entry_regs():
    mlir = _compile_lq(name="lq_rc").emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    # 2 ptrs + 5 Vector entry fields
    assert n >= 7, f"LoadQueue needs ptr + entry vector regs; got {n}"
    assert "vector<" in mlir


if __name__ == "__main__":
    for s in (4, 8):
        test_load_queue_small_emit_mlir(s)
        print(f"PASS: test_load_queue_small_emit_mlir size={s}")
    test_load_queue_has_entry_regs()
    print("PASS: test_load_queue_has_entry_regs")
