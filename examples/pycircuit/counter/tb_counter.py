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

from counter import build  # noqa: E402
from counter_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    p = TB_PRESETS["smoke"]
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(int(p["timeout"]))

    # --- cycle 0 ---
    tb.drive("enable", 1)
    tb.expect("count", 1)

    tb.next()  # --- cycle 1 ---
    tb.expect("count", 2)

    tb.next()  # --- cycle 2 ---
    tb.expect("count", 3)

    tb.next()  # --- cycle 3 ---
    tb.expect("count", 4)

    tb.next()  # --- cycle 4 ---
    tb.expect("count", 5)

    tb.finish(at=int(p["finish"]))


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="tb_counter_top", eager=True, **DEFAULT_PARAMS
        ).emit_mlir()
    )
