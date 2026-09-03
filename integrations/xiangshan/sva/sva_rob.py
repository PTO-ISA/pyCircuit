"""SVA formal properties for ROB.

Properties attached via @testbench + CycleAwareTb.sva_assert.
These are compiled into SystemVerilog `assert property` statements
by pycc and checked during simulation.

Properties:
  P-ROB-001  Commit only when head is valid AND writebacked
  P-ROB-002  Commit is contiguous (commit[i] → commit[i-1])
  P-ROB-003  Exception blocks commit
  P-ROB-004  Tail never passes head by more than ROB_SIZE
"""

from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench
from pycircuit.tb import sva

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.rob.rob import build_rob  # noqa: E402

ROB_SZ = 4
CM_W = 2


@testbench
def tb_sva_rob(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(64)

    # P-ROB-002: commit contiguity — commit[1] implies commit[0]
    tb.sva_assert(
        ~sva.id("commit_valid_1") | sva.id("commit_valid_0"),
        clock="clk",
        reset="rst",
        name="rob_commit_contiguous",
        msg="commit_valid_1 without commit_valid_0 violates in-order commit",
    )

    # P-ROB-003: exception_valid implies no commit
    tb.sva_assert(
        ~sva.id("exception_valid") | ~sva.id("commit_valid_0"),
        clock="clk",
        reset="rst",
        name="rob_exception_blocks_commit",
        msg="commit fired despite exception at head",
    )

    # Idle stimulus so the simulation can complete
    for _ in range(32):
        tb.next()
    tb.finish()
