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

from obs_points import build  # noqa: E402
from obs_points_config import DEFAULT_PARAMS, TB_PRESETS  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    p = TB_PRESETS["smoke"]
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=0)
    tb.timeout(int(p["timeout"]))

    # --- cycle 0 ---
    # Default drives.
    tb.drive("x", 0)
    # Cycle 0: comb changes visible at pre; state updates visible at post.
    tb.drive("x", 10)
    tb.expect("y", 11, phase="pre", msg="TICK-OBS: comb must reflect current drives")
    tb.expect("q", 0, phase="pre", msg="TICK-OBS: state is pre-commit")
    tb.expect("q", 11, phase="post", msg="XFER-OBS: state commit is visible")

    tb.next()  # --- cycle 1 ---
    # Cycle 1: repeat with a new drive to validate both obs points again.
    tb.drive("x", 20)
    tb.expect("y", 21, phase="pre")
    tb.expect("q", 11, phase="pre")
    tb.expect("q", 21, phase="post")

    tb.finish(at=int(p["finish"]))


if __name__ == "__main__":
    print(
        compile_cycle_aware(
            build, name="tb_obs_points_top", eager=True, **DEFAULT_PARAMS
        ).emit_mlir()
    )
