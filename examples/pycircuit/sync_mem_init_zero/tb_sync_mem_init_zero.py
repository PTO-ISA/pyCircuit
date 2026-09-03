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

from sync_mem_init_zero import build  # noqa: E402
from sync_mem_init_zero_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    p = TB_PRESETS["smoke"]
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(int(p["timeout"]))

    # --- cycle 0 ---
    # Default drives (no writes).
    tb.drive("wvalid", 0)
    tb.drive("waddr", 0)
    tb.drive("wdata", 0)
    tb.drive("wstrb", 0)

    # Read from unwritten addresses: deterministic sim init must be 0.
    tb.drive("ren", 1)
    tb.drive("raddr", 1)
    tb.expect(
        "rdata",
        0,
        phase="post",
        msg="sync_mem must initialize entries to 0 (deterministic sim)",
    )

    tb.next()  # --- cycle 1 ---
    tb.drive("ren", 1)
    tb.drive("raddr", 3)
    tb.expect("rdata", 0, phase="post")

    tb.finish(at=int(p["finish"]))


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="tb_sync_mem_init_zero_top", eager=True, **DEFAULT_PARAMS
        ).emit_mlir()
    )
