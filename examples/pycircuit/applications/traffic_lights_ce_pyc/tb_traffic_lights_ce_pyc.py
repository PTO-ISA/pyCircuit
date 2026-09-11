from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware
from pycircuit.design import testbench

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from traffic_lights_ce_pyc import build  # noqa: E402
from traffic_lights_ce_pyc_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    schedule = CycleAwareTb(t)
    preset = TB_PRESETS["smoke"]
    schedule.clock("clk")
    schedule.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    schedule.timeout(int(preset["timeout"]))
    schedule.drive("go", 1)
    schedule.drive("emergency", 0)
    schedule.finish(at=int(preset["finish"]))


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="tb_traffic_lights_ce_pyc_top", **DEFAULT_PARAMS
        ).emit_mlir()
    )
