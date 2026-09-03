"""Testbench for IssueQueue — MLIR smoke (L1) + functional directed (L2).

Vector ports use lane lists (auto-packed by CycleAwareTb, lane0 = LSB).
Expects use phase='pre' because issue/dequeue are combinational from Q.
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

from backend.issue.issue_queue import issue_queue  # noqa: E402

ENTRIES = 4
ENQ_P = 2
ISSUE_P = 1
WB_P = 2
PTAG_W = 4
ROB_W = 4
FU_W = 3
P = "iq"


def _compile_iq(
    *, entries: int = ENTRIES, name: str = "iq_s", issue_ports: int = ISSUE_P
):
    return compile_cycle_aware(
        issue_queue,
        name=name,
        eager=True,
        prefix=P,
        entries=entries,
        enq_ports=ENQ_P,
        issue_ports=issue_ports,
        wb_ports=WB_P,
        ptag_w=PTAG_W,
        rob_idx_w=ROB_W,
        fu_type_width=FU_W,
    )


def _drive_idle(tb: CycleAwareTb) -> None:
    """Drive all inputs to zero. Vector ports take lane lists; scalars take 0."""
    z_enq = [0] * ENQ_P
    z_wb = [0] * WB_P
    tb.drive("iq_enq_valid", z_enq)
    tb.drive("iq_enq_pdest", z_enq)
    tb.drive("iq_enq_psrc1", z_enq)
    tb.drive("iq_enq_psrc2", z_enq)
    tb.drive("iq_enq_src1_ready", z_enq)
    tb.drive("iq_enq_src2_ready", z_enq)
    tb.drive("iq_enq_rob_idx", z_enq)
    tb.drive("iq_enq_fu_type", z_enq)
    tb.drive("iq_wb_valid", z_wb)
    tb.drive("iq_wb_pdest", z_wb)
    tb.drive("iq_flush", 0)


@testbench
def tb_issue_queue_functional(t: Tb) -> None:
    # Compile the device and bind it so vector ports accept lane lists.
    circuit = _compile_iq(name="iq_tb")
    tb = CycleAwareTb(t, circuit=circuit)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(40)

    _drive_idle(tb)
    tb.expect("iq_issue_valid", [0], phase="pre", msg="T1: no issue when empty")
    tb.expect("iq_ready", 1, phase="pre", msg="T1: has room")
    tb.expect("iq_free_count", ENTRIES, phase="pre", msg=f"T1: {ENTRIES} free")

    tb.next()
    _drive_idle(tb)
    tb.drive("iq_enq_valid", [1, 0])
    tb.drive("iq_enq_pdest", [5, 0])
    tb.drive("iq_enq_psrc1", [1, 0])
    tb.drive("iq_enq_psrc2", [2, 0])
    tb.drive("iq_enq_src1_ready", [1, 0])
    tb.drive("iq_enq_src2_ready", [1, 0])
    tb.drive("iq_enq_rob_idx", [1, 0])
    tb.drive("iq_enq_fu_type", [0, 0])
    tb.expect("iq_issue_valid", [0], phase="pre", msg="T2: no issue on enqueue cycle")

    tb.next()
    _drive_idle(tb)
    tb.expect("iq_issue_valid", [1], phase="pre", msg="T2: issue after 1 cycle")
    tb.expect("iq_issue_pdest", [5], phase="pre", msg="T2: correct pdest")
    tb.expect("iq_issue_rob_idx", [1], phase="pre", msg="T2: correct rob_idx")
    tb.expect("iq_issue_fu_type", [0], phase="pre", msg="T2: correct fu_type")

    tb.next()
    _drive_idle(tb)
    tb.drive("iq_enq_valid", [1, 1])
    tb.drive("iq_enq_pdest", [6, 7])
    tb.drive("iq_enq_psrc1", [1, 6])
    tb.drive("iq_enq_psrc2", [2, 2])
    tb.drive("iq_enq_src1_ready", [1, 0])
    tb.drive("iq_enq_src2_ready", [1, 1])
    tb.drive("iq_enq_rob_idx", [2, 3])
    tb.drive("iq_enq_fu_type", [0, 1])
    tb.expect(
        "iq_issue_valid", [0], phase="pre", msg="T3: previous op drained; enqueue cycle"
    )

    tb.next()
    _drive_idle(tb)
    tb.expect("iq_issue_valid", [1], phase="pre", msg="T3: A issues (both srcs ready)")
    tb.expect("iq_issue_pdest", [6], phase="pre", msg="T3: A pdest=6")

    tb.next()
    _drive_idle(tb)
    tb.drive("iq_wb_valid", [1, 0])
    tb.drive("iq_wb_pdest", [6, 0])
    tb.expect("iq_issue_valid", [1], phase="pre", msg="T3: B issues after wakeup")
    tb.expect("iq_issue_pdest", [7], phase="pre", msg="T3: B pdest=7")

    tb.next()
    _drive_idle(tb)
    tb.drive("iq_enq_valid", [1, 0])
    tb.drive("iq_enq_pdest", [8, 0])
    tb.drive("iq_enq_psrc1", [10, 0])
    tb.drive("iq_enq_psrc2", [2, 0])
    tb.drive("iq_enq_src1_ready", [0, 0])
    tb.drive("iq_enq_src2_ready", [1, 0])
    tb.drive("iq_enq_rob_idx", [4, 0])
    tb.drive("iq_enq_fu_type", [1, 0])

    tb.next()
    _drive_idle(tb)
    tb.drive("iq_enq_valid", [1, 0])
    tb.drive("iq_enq_pdest", [9, 0])
    tb.drive("iq_enq_psrc1", [10, 0])
    tb.drive("iq_enq_psrc2", [2, 0])
    tb.drive("iq_enq_src1_ready", [0, 0])
    tb.drive("iq_enq_src2_ready", [1, 0])
    tb.drive("iq_enq_rob_idx", [5, 0])
    tb.drive("iq_enq_fu_type", [1, 0])
    tb.expect("iq_issue_valid", [0], phase="pre", msg="T4: C/D not ready, no issue")

    tb.next()
    _drive_idle(tb)
    tb.drive("iq_wb_valid", [1, 0])
    tb.drive("iq_wb_pdest", [10, 0])
    tb.expect("iq_issue_valid", [1], phase="pre", msg="T4: oldest-ready issues")
    tb.expect("iq_issue_pdest", [8], phase="pre", msg="T4: C (older) pdest=8")

    tb.next()
    _drive_idle(tb)
    tb.expect("iq_issue_valid", [1], phase="pre", msg="T4: D issues next cycle")
    tb.expect("iq_issue_pdest", [9], phase="pre", msg="T4: D pdest=9")

    tb.next()
    _drive_idle(tb)
    tb.drive("iq_enq_valid", [1, 0])
    tb.drive("iq_enq_pdest", [11, 0])
    tb.drive("iq_enq_src1_ready", [1, 0])
    tb.drive("iq_enq_src2_ready", [1, 0])

    tb.next()
    _drive_idle(tb)
    tb.drive("iq_flush", 1)
    tb.expect("iq_issue_valid", [0], phase="pre", msg="T5: flush suppresses issue")
    tb.expect("iq_ready", 0, phase="pre", msg="T5: not ready while flush")

    tb.next()
    _drive_idle(tb)
    tb.expect("iq_issue_valid", [0], phase="pre", msg="T5: empty after flush")
    tb.expect("iq_free_count", ENTRIES, phase="pre", msg="T5: all free after flush")

    tb.finish()


@pytest.mark.smoke
def test_issue_queue_emit_mlir():
    mlir = _compile_iq(entries=8, name="issue_queue", issue_ports=2).emit_mlir()
    assert "func.func" in mlir
    assert "issue_valid" in mlir
    assert "vector<" in mlir
    assert "enq_valid_0" not in mlir
    assert "issue_valid_0" not in mlir


@pytest.mark.smoke
def test_issue_queue_small_emit_mlir():
    mlir = _compile_iq(name="iq_s").emit_mlir()
    assert "func.func" in mlir
    assert "issue_valid" in mlir
    assert "vector<" in mlir
    assert "enq_valid_0" not in mlir


@pytest.mark.regcount
def test_issue_queue_has_state_regs():
    mlir = _compile_iq(name="iq_rc").emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    assert n >= 1, f"IssueQueue must keep state regs; got {n}"
    assert "vector<" in mlir


if __name__ == "__main__":
    test_issue_queue_small_emit_mlir()
    print("PASS: test_issue_queue_small_emit_mlir")
    test_issue_queue_emit_mlir()
    print("PASS: test_issue_queue_emit_mlir")
    test_issue_queue_has_state_regs()
    print("PASS: test_issue_queue_has_state_regs")
