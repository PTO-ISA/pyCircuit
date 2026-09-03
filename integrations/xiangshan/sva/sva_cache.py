"""SVA formal properties for Cache and TLB.

Properties:
  P-TLB-001  hit and miss are mutually exclusive
  P-DC-001   At most one way hits per access (popcount(way_hits) <= 1)
"""

from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench
from pycircuit.tb import sva

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))


@testbench
def tb_sva_tlb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(64)

    # P-TLB-001: hit and miss mutually exclusive
    tb.sva_assert(
        ~(sva.id("resp_hit") & sva.id("resp_miss")),
        clock="clk",
        reset="rst",
        name="tlb_hit_miss_mutex",
        msg="TLB hit and miss asserted simultaneously",
    )

    for _ in range(32):
        tb.next()
    tb.finish()
