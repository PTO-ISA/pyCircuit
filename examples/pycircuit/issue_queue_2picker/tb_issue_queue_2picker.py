from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareTb,
    Tb,
    build_cycle_aware,
)
from pycircuit.design import testbench

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from issue_queue_2picker import build  # noqa: E402
from issue_queue_2picker_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    p = TB_PRESETS["smoke"]
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(int(p["timeout"]))
    # --- cycle 0 ---
    tb.drive("in_valid", 0)
    tb.drive("in_data", 0)
    tb.drive("out0_ready", 0)
    tb.drive("out1_ready", 0)
    tb.expect("in_ready", 1)
    tb.finish(at=int(p["finish"]))


if __name__ == "__main__":
    print(
        build_cycle_aware(
            build, name="tb_issue_queue_2picker_top", **DEFAULT_PARAMS
        ).emit_mlir()
    )
