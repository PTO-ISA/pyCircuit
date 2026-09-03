from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import CycleAwareCircuit, Tb, module, testbench

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from bypass_unit_v6 import PTYPE_P, bypass_unit  # noqa: E402

LANES = 4
DATA_W = 16
PTAG_COUNT = 16
PTYPE_COUNT = 4
PTAG_W = max(1, (PTAG_COUNT - 1).bit_length())
PTYPE_W = max(1, (PTYPE_COUNT - 1).bit_length())
LANE_W = max(1, (LANES - 1).bit_length())


def _pack(values: list[int], width: int) -> int:
    out = 0
    mask = (1 << int(width)) - 1
    for i, value in enumerate(values):
        out |= (int(value) & mask) << (i * int(width))
    return out


def _drive_zero(tb) -> None:
    for stage in ("w1", "w2", "w3"):
        tb.drive(f"bp_{stage}_valid", 0)
        tb.drive(f"bp_{stage}_ptag", 0)
        tb.drive(f"bp_{stage}_ptype", 0)
        tb.drive(f"bp_{stage}_data", 0)
    for src in ("srcL", "srcR"):
        tb.drive(f"bp_i2_{src}_valid", 0)
        tb.drive(f"bp_i2_{src}_ptag", 0)
        tb.drive(f"bp_i2_{src}_ptype", 0)
        tb.drive(f"bp_i2_{src}_rf_data", 0)


@module
def build(
    m: CycleAwareCircuit,
    lanes: int = LANES,
    data_width: int = DATA_W,
    ptag_count: int = PTAG_COUNT,
    ptype_count: int = PTYPE_COUNT,
) -> None:
    domain = m.create_domain("clk")
    bypass_unit(
        m,
        domain,
        prefix="bp",
        lanes=lanes,
        data_width=data_width,
        ptag_count=ptag_count,
        ptype_count=ptype_count,
    )


@testbench
def tb(t: Tb) -> None:
    from pycircuit import CycleAwareTb

    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=1, cycles_deasserted=1)
    tb.timeout(16)

    _drive_zero(tb)
    tb.next()
    _drive_zero(tb)
    tb.next()

    # Issue a source and make w3 match first.
    _drive_zero(tb)
    tb.drive("bp_i2_srcL_valid", _pack([1, 0, 0, 0], 1))
    tb.drive("bp_i2_srcL_ptag", _pack([10, 0, 0, 0], PTAG_W))
    tb.drive("bp_i2_srcL_ptype", _pack([PTYPE_P, 0, 0, 0], PTYPE_W))
    tb.drive("bp_i2_srcL_rf_data", _pack([0x0AAA, 0, 0, 0], DATA_W))
    tb.drive("bp_w3_valid", _pack([0, 0, 1, 0], 1))
    tb.drive("bp_w3_ptag", _pack([0, 0, 10, 0], PTAG_W))
    tb.drive("bp_w3_ptype", _pack([0, 0, PTYPE_P, 0], PTYPE_W))
    tb.drive("bp_w3_data", _pack([0, 0, 0x1111, 0], DATA_W))
    tb.next()

    # In the next visible TB cycle, w1 has highest priority over w2/w3.
    _drive_zero(tb)
    tb.drive("bp_w2_valid", _pack([0, 1, 0, 0], 1))
    tb.drive("bp_w2_ptag", _pack([0, 10, 0, 0], PTAG_W))
    tb.drive("bp_w2_ptype", _pack([0, PTYPE_P, 0, 0], PTYPE_W))
    tb.drive("bp_w2_data", _pack([0, 0x2222, 0, 0], DATA_W))
    tb.drive("bp_w1_valid", _pack([0, 0, 0, 1], 1))
    tb.drive("bp_w1_ptag", _pack([0, 0, 0, 10], PTAG_W))
    tb.drive("bp_w1_ptype", _pack([0, 0, 0, PTYPE_P], PTYPE_W))
    tb.drive("bp_w1_data", _pack([0, 0, 0, 0x3333], DATA_W))

    tb.expect("bp_i2_srcL_data", _pack([0x3333, 0, 0, 0], DATA_W), msg="v6 data")
    tb.expect("bp_i2_srcL_hit", _pack([1, 0, 0, 0], 1), msg="v6 hit")
    tb.expect("bp_i2_srcL_sel_stage", _pack([1, 0, 0, 0], 2), msg="v6 stage")
    tb.expect("bp_i2_srcL_sel_lane", _pack([3, 0, 0, 0], LANE_W), msg="v6 lane")
    tb.finish()
