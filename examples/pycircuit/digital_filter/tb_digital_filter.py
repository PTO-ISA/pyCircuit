from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from digital_filter import build  # noqa: E402
from digital_filter_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    p = TB_PRESETS["smoke"]
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(int(p["timeout"]))

    # First valid sample with zero delay line: y = c0*x = 1*1
    tb.drive("x_in", 1)
    tb.drive("x_valid", 1)
    tb.expect("y_out", 1)
    tb.expect("y_valid", 1)

    tb.finish(at=int(p["finish"]))


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="tb_digital_filter_top", eager=True, **DEFAULT_PARAMS
        ).emit_mlir()
    )
