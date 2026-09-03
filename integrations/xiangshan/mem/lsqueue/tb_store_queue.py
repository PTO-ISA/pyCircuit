"""Testbench for Store Queue — MLIR smoke (L1) + functional directed (L2).

Uses tiny power-of-two config: size=4, addr_width=16, data_width=16.
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

from mem.lsqueue.store_queue import store_queue  # noqa: E402

SQ_SZ = 4
ADDR_W = 16
DATA_W = 16
ROB_IDX_W = 4
P = "stq"


def _compile_sq(*, size: int = SQ_SZ, name: str = "sq_s"):
    return compile_cycle_aware(
        store_queue,
        name=name,
        eager=True,
        prefix=P,
        size=size,
        addr_width=ADDR_W,
        data_width=DATA_W,
        rob_idx_width=ROB_IDX_W,
    )


def _zero_inputs(tb: CycleAwareTb) -> None:
    tb.drive("stq_flush", 0)
    tb.drive("stq_enq_valid", 0)
    tb.drive("stq_enq_rob_idx", 0)
    tb.drive("stq_write_valid", 0)
    tb.drive("stq_write_idx", 0)
    tb.drive("stq_write_addr", 0)
    tb.drive("stq_write_data", 0)
    tb.drive("stq_commit_valid", 0)
    tb.drive("stq_fwd_valid", 0)
    tb.drive("stq_fwd_addr", 0)
    tb.drive("stq_sbuf_ready", 0)
    tb.drive("stq_redirect_valid", 0)


@testbench
def tb_store_queue_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(50)

    _zero_inputs(tb)
    tb.expect("stq_count", 0, msg="T1: empty after reset")

    tb.next()
    _zero_inputs(tb)
    tb.drive("stq_enq_valid", 1)
    tb.drive("stq_enq_rob_idx", 3)
    tb.expect("stq_can_enqueue", 1, msg="T1: can enqueue when space available")
    tb.expect("stq_enq_idx", 0, msg="T1: enqueue at idx 0")

    tb.next()
    _zero_inputs(tb)
    tb.expect("stq_count", 1, msg="T1: count=1 after enqueue")

    tb.drive("stq_write_valid", 1)
    tb.drive("stq_write_idx", 0)
    tb.drive("stq_write_addr", 0x1000)
    tb.drive("stq_write_data", 0xBEEF)

    tb.next()
    _zero_inputs(tb)

    tb.drive("stq_fwd_valid", 1)
    tb.drive("stq_fwd_addr", 0x1000)
    tb.expect("stq_fwd_hit", 1, msg="T2: forwarding hit (same line)")
    tb.expect("stq_fwd_data", 0xBEEF, msg="T2: forwarded data matches")

    tb.next()
    _zero_inputs(tb)

    tb.drive("stq_fwd_valid", 1)
    tb.drive("stq_fwd_addr", 0x2000)
    tb.expect("stq_fwd_hit", 0, msg="T2: no forwarding hit (different line)")

    tb.next()
    _zero_inputs(tb)
    tb.drive("stq_commit_valid", 1)

    tb.next()
    _zero_inputs(tb)
    tb.drive("stq_sbuf_ready", 1)
    tb.expect("stq_sbuf_valid", 1, msg="T3: drain valid after commit")
    tb.expect("stq_sbuf_data", 0xBEEF, msg="T3: drain data matches")

    tb.next()
    _zero_inputs(tb)
    tb.expect("stq_count", 0, msg="T3: count=0 after drain")

    tb.finish()


@pytest.mark.smoke
@pytest.mark.parametrize("size", [4, 8])
def test_store_queue_small_emit_mlir(size: int):
    mlir = _compile_sq(size=size, name=f"sq_s{size}").emit_mlir()
    assert "func.func" in mlir
    assert "can_enqueue" in mlir
    assert "fwd_hit" in mlir
    assert "sbuf_valid" in mlir
    assert "vector<" in mlir


@pytest.mark.regcount
def test_store_queue_has_entry_regs():
    mlir = _compile_sq(name="sq_rc").emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    # 3 ptrs + 6 Vector entry fields
    assert n >= 9, f"StoreQueue needs ptr + entry vector regs; got {n}"
    assert "vector<" in mlir


if __name__ == "__main__":
    for s in (4, 8):
        test_store_queue_small_emit_mlir(s)
        print(f"PASS: test_store_queue_small_emit_mlir size={s}")
    test_store_queue_has_entry_regs()
    print("PASS: test_store_queue_has_entry_regs")
