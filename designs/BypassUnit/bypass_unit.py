from __future__ import annotations

from pycircuit import (
    Circuit,
    Tb,
    Wire,
    compile,
    function,
    module,
    testbench,
)
from pycircuit.data import Bits, Data, Vector
from pycircuit.v6 import mux

PTYPE_C = 0
PTYPE_P = 1
PTYPE_T = 2
PTYPE_U = 3


@function
def _not1(m, x):
    return m.const(1, width=1) ^ x


@function
def _select_stage_batch(
    m: Circuit,
    *,
    src_valid_v: Wire[Vector[Data]],
    src_ptag_v: Wire[Vector[Data]],
    src_ptype_v: Wire[Vector[Data]],
    lane_valid: Wire[Vector[Data]],
    lane_ptag: Wire[Vector[Data]],
    lane_ptype: Wire[Vector[Data]],
    lane_data: Wire[Vector[Data]],
    lane_nums: Wire[Vector[Bits]],
    zero_lane: Wire[Bits],
    zero_data: Wire[Bits],
    lanes_n: int,
):
    """Batch bypass search: N sources × M lanes outer-product, one call."""
    _ = m
    n = int(lanes_n)
    # Outer product via broadcast.
    # src: Wire<N> → broadcast dim=1, size=M → Wire<N, M> (row-replicated)
    # lane: Wire<M> → broadcast dim=0, size=N → Wire<N, M> (col-replicated)
    sv_bc = src_valid_v.broadcast(dim=1, size=n)
    sp_bc = src_ptag_v.broadcast(dim=1, size=n)
    st_bc = src_ptype_v.broadcast(dim=1, size=n)

    lv_bc = lane_valid.broadcast(dim=0, size=n)
    lp_bc = lane_ptag.broadcast(dim=0, size=n)
    lt_bc = lane_ptype.broadcast(dim=0, size=n)

    match: Wire[Vector[Vector[Data]]] = (
        sv_bc & lv_bc & (lp_bc == sp_bc) & (lt_bc == st_bc)
    )
    # match: Wire<Wire<i1, M>, N> — N rows, each row is M match bits

    has = match.reduce_or(dim=1)  # Wire<i1, N>: per-source hit

    # Per-source priority mux over the match row.
    sel_lane = m.vec(
        [m.priority_mux(match[i], lane_nums, default=zero_lane) for i in range(n)]
    )
    # Broadcast lane_data to match shape, then per-row priority mux.
    ld_bc: Wire[Vector[Vector[Data]]] = lane_data.broadcast(dim=0, size=n)
    sel_data = m.vec(
        [m.priority_mux(match[i], ld_bc[i], default=zero_data) for i in range(n)]
    )

    return has, sel_lane, sel_data


@module
def build(
    m: Circuit,
    *,
    lanes: int = 8,
    data_width: int = 64,
    ptag_count: int = 256,
    ptype_count: int = 4,
) -> None:
    lanes_n = int(lanes)
    data_w = int(data_width)
    ptag_n = int(ptag_count)
    ptype_n = int(ptype_count)

    if lanes_n <= 0:
        raise ValueError("bypass_unit lanes must be > 0")
    if data_w <= 0:
        raise ValueError("bypass_unit data_width must be > 0")
    if ptag_n <= 0:
        raise ValueError("bypass_unit ptag_count must be > 0")
    if ptype_n <= 0:
        raise ValueError("bypass_unit ptype_count must be > 0")
    if ptype_n <= PTYPE_U:
        raise ValueError("bypass_unit ptype_count must be >= 4 to represent C/P/T/U")

    ptag_w = max(1, (ptag_n - 1).bit_length())
    ptype_w = max(1, (ptype_n - 1).bit_length())
    lane_w = max(1, (lanes_n - 1).bit_length())

    # Pre-compute constants for the vectorised bypass search.
    # Use m.const: u() returns LiteralValue which Wire cannot ingest.
    lane_nums = m.vec([m.const(j, width=lane_w) for j in range(int(lanes_n))])
    zero_hit = m.const(0, width=1)
    one_hit = m.const(1, width=1)
    zero_stage = m.const(0, width=2)
    stage_consts = {
        1: m.const(1, width=2),
        2: m.const(2, width=2),
        3: m.const(3, width=2),
    }
    zero_lane = m.const(0, width=lane_w)
    zero_data = m.const(0, width=data_w)

    # Write-back stages as Vecs (one Wire per stage, size = lanes).
    w_valid: dict[str, Wire[Vector[Data]]] = {}
    w_ptag: dict[str, Wire[Vector[Data]]] = {}
    w_ptype: dict[str, Wire[Vector[Data]]] = {}
    w_data: dict[str, Wire[Vector[Data]]] = {}
    for stage in ("w1", "w2", "w3"):
        w_valid[stage] = m.input(f"{stage}_valid", width=1, shape=[lanes_n])
        w_ptag[stage] = m.input(f"{stage}_ptag", width=ptag_w, shape=[lanes_n])
        w_ptype[stage] = m.input(f"{stage}_ptype", width=ptype_w, shape=[lanes_n])
        w_data[stage] = m.input(f"{stage}_data", width=data_w, shape=[lanes_n])

    for src in ("srcL", "srcR"):
        # Collect all N sources of this type into Vecs.
        src_valid_v = m.input(f"i2_{src}_valid", width=1, shape=[lanes_n])
        src_ptag_v = m.input(f"i2_{src}_ptag", width=ptag_w, shape=[lanes_n])
        src_ptype_v = m.input(f"i2_{src}_ptype", width=ptype_w, shape=[lanes_n])
        src_rfdata_v = m.input(f"i2_{src}_rf_data", width=data_w, shape=[lanes_n])

        sel_data: Wire[Data] = src_rfdata_v
        sel_hit: Wire[Data] = m.vec([zero_hit for _ in range(lanes_n)])
        sel_stage: Wire[Data] = m.vec([zero_stage for _ in range(lanes_n)])
        sel_lane: Wire[Data] = m.vec([zero_lane for _ in range(lanes_n)])

        # Priority: w3 > w2 > w1.
        for stage, prio in [("w3", 3), ("w2", 2), ("w1", 1)]:
            has, lane_sel, data_sel = _select_stage_batch(
                m,
                src_valid_v=src_valid_v,
                src_ptag_v=src_ptag_v,
                src_ptype_v=src_ptype_v,
                lane_valid=w_valid[stage],
                lane_ptag=w_ptag[stage],
                lane_ptype=w_ptype[stage],
                lane_data=w_data[stage],
                lane_nums=lane_nums,
                zero_lane=zero_lane,
                zero_data=zero_data,
                lanes_n=lanes_n,
            )
            sel_data = mux(has, data_sel, sel_data)
            sel_hit = mux(has, one_hit, sel_hit)
            sel_stage = mux(has, stage_consts[prio], sel_stage)
            sel_lane = mux(has, lane_sel, sel_lane)

        m.output(f"i2_{src}_data", sel_data)
        m.output(f"i2_{src}_hit", sel_hit)
        m.output(f"i2_{src}_sel_stage", sel_stage)
        m.output(f"i2_{src}_sel_lane", sel_lane)


build.__pycircuit_name__ = "bypass_unit"  # type: ignore[union-attr]


@testbench
def tb(t: Tb) -> None:
    t.clock("clk")
    t.reset("rst", cycles_asserted=1, cycles_deasserted=1)
    t.timeout(4)
    t.finish(at=0)


if __name__ == "__main__":
    print(
        compile(
            build,
            name="bypass_unit",
            lanes=8,
            data_width=64,
            ptag_count=256,
            ptype_count=4,
        ).emit_mlir()[:500]
    )
