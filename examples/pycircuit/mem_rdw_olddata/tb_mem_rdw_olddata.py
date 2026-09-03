from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareTb,
    Tb,
    compile_cycle_aware,
    testbench,
)

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from mem_rdw_olddata import build  # noqa: E402
from mem_rdw_olddata_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    p = TB_PRESETS["smoke"]
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(int(p["timeout"]))

    # --- cycle 0 ---
    # Default drives.
    tb.drive("ren", 0)
    tb.drive("raddr", 0)
    tb.drive("wvalid", 0)
    tb.drive("waddr", 0)
    tb.drive("wdata", 0)
    tb.drive("wstrb", 0)

    # Cycle 0: write old value.
    tb.drive("wvalid", 1)
    tb.drive("waddr", 0)
    tb.drive("wdata", 0x11111111)
    tb.drive("wstrb", 0xF)

    tb.next()  # --- cycle 1 ---
    # Cycle 1: read+write same address -> expect old-data.
    tb.drive("ren", 1)
    tb.drive("raddr", 0)
    tb.drive("wvalid", 1)
    tb.drive("waddr", 0)
    tb.drive("wdata", 0x22222222)
    tb.drive("wstrb", 0xF)
    tb.expect("rdata", 0x11111111, phase="post", msg="RDW must return old-data")

    tb.next()  # --- cycle 2 ---
    # Cycle 2: read again -> expect new data.
    tb.drive("wvalid", 0)
    tb.drive("wstrb", 0)
    tb.drive("ren", 1)
    tb.drive("raddr", 0)
    tb.expect("rdata", 0x22222222, phase="post")

    tb.finish(at=int(p["finish"]))


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="tb_mem_rdw_olddata_top", eager=True, **DEFAULT_PARAMS
        ).emit_mlir()
    )
