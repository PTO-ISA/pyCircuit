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

from hier_modules import build  # noqa: E402
from hier_modules_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    p = TB_PRESETS["smoke"]
    tb.timeout(int(p["timeout"]))

    # --- cycle 0 ---
    tb.drive("x", 1)
    tb.expect("y", 4)

    tb.finish(at=int(p["finish"]))


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="tb_hier_modules_top", eager=True, **DEFAULT_PARAMS
        ).emit_mlir()
    )
