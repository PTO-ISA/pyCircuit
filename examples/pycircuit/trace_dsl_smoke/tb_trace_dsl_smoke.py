from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import (
    CycleAwareTb,
    Tb,
    compile_cycle_aware,
    testbench,
)

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from trace_dsl_smoke import build  # noqa: E402
from trace_dsl_smoke_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    p = TB_PRESETS["smoke"]
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=0)
    tb.timeout(int(p["timeout"]))

    # Cycle 0: pre is init, post sees commit.
    tb.drive("in_x", 0x12)
    tb.expect("y0", 0x00, phase="pre")
    tb.expect("y1", 0x00, phase="pre")
    tb.expect("y0", 0x12, phase="post")
    tb.expect("y1", 0x12, phase="post")

    tb.next()  # Cycle 1: same behavior with a new drive.
    tb.drive("in_x", 0x34)
    tb.expect("y0", 0x12, phase="pre")
    tb.expect("y1", 0x12, phase="pre")
    tb.expect("y0", 0x34, phase="post")
    tb.expect("y1", 0x34, phase="post")

    tb.next()  # Cycle 2: stable drive; trace still records Write intent.
    tb.drive("in_x", 0x34)
    tb.expect("y0", 0x34, phase="pre")
    tb.expect("y1", 0x34, phase="pre")
    tb.expect("y0", 0x34, phase="post")
    tb.expect("y1", 0x34, phase="post")

    tb.finish(at=int(p["finish"]))


if __name__ == "__main__":
    sys.stdout.write(
        compile_cycle_aware(
            build, name="tb_trace_dsl_smoke_top", **DEFAULT_PARAMS
        ).emit_mlir()
        + "\n"
    )
