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

from xz_value_model_smoke import build  # noqa: E402
from xz_value_model_smoke_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    p = TB_PRESETS["smoke"]
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=0)
    tb.timeout(int(p["timeout"]))

    # --- cycle 0 ---
    tb.drive("in_a", 0x12)
    tb.expect("y", 0x00, phase="pre")
    tb.expect("y", 0x12, phase="post")

    tb.next()  # --- cycle 1 ---
    tb.drive("in_a", 0x56)
    tb.expect("y", 0x12, phase="pre")
    tb.expect("y", 0x56, phase="post")

    tb.finish(at=int(p["finish"]))


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="tb_xz_value_model_smoke_top", eager=True, **DEFAULT_PARAMS
        ).emit_mlir()
    )
