from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware
from pycircuit.design import testbench

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from bf16_fmac import build  # noqa: E402
from bf16_fmac_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    schedule = CycleAwareTb(t)
    preset = TB_PRESETS["smoke"]
    schedule.clock("clk")
    schedule.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    schedule.timeout(int(preset["timeout"]))
    schedule.drive("a_in", 0)
    schedule.drive("b_in", 0)
    schedule.drive("acc_in", 0)
    schedule.drive("valid_in", 0)
    schedule.finish(at=int(preset["finish"]))


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="tb_bf16_fmac_top", **DEFAULT_PARAMS
        ).emit_mlir()
    )
