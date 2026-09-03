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

from multiclock_regs import build  # noqa: E402
from multiclock_regs_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    p = TB_PRESETS["smoke"]
    tb.clock("clk_a")
    tb.clock("clk_b")
    tb.reset("rst_a", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(int(p["timeout"]))
    # --- cycle 0 ---
    tb.drive("rst_b", 0)
    tb.finish(at=int(p["finish"]))


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="tb_multiclock_regs_top", **DEFAULT_PARAMS
        ).emit_mlir()
    )
