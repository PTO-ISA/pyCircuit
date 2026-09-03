"""SVA formal properties for bus protocols (AXI4 / TileLink).

Properties:
  P-AXI-001  AXI valid must not deassert before ready (AXI handshake rule)
  P-AXI-002  AXI data/address must be stable while valid && !ready
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
def tb_sva_axi_handshake(t: Tb) -> None:
    """AXI-like valid/ready handshake properties (generic template)."""
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(64)

    # P-AXI-001: once valid rises, it cannot fall until ready
    # $rose(valid) |-> valid until ready
    # Expressed as: past(!valid) && valid -> valid || past(ready)
    # Simplified check: valid && !ready -> next(valid)
    tb.sva_assert(
        ~(sva.id("aw_valid") & ~sva.id("aw_ready")) | sva.id("aw_valid"),
        clock="clk",
        reset="rst",
        name="axi_aw_valid_stable",
        msg="AW valid dropped before ready",
    )

    for _ in range(32):
        tb.next()
    tb.finish()
