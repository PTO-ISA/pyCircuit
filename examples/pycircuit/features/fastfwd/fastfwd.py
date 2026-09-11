from __future__ import annotations

from pycircuit import Circuit, CycleAwareCircuit, CycleAwareDomain, const, ct, u


@const
def _total_engines(m: Circuit, n_fe: int | None, eng_per_lane: int) -> int:
    _ = m
    if n_fe is not None:
        n = int(n_fe)
        if n <= 0:
            raise ValueError("N_FE must be > 0")
        return n
    return max(1, int(eng_per_lane)) * ct.div_ceil(4, 1)


def build(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    N_FE: int | None = None,
    ENG_PER_LANE: int = 1,
    LANE_Q_DEPTH: int = 16,
    ENG_Q_DEPTH: int = 4,
    ROB_DEPTH: int = 16,
    SEQ_W: int = 16,
    HIST_DEPTH: int = 8,
    STASH_WIN: int = 6,
    BKPR_SLACK: int = 1,
) -> None:
    _ = domain
    _ = (LANE_Q_DEPTH, ENG_Q_DEPTH, ROB_DEPTH, SEQ_W, HIST_DEPTH, STASH_WIN, BKPR_SLACK)
    total_eng = _total_engines(m, N_FE, ENG_PER_LANE)

    lane_in_vld = [m.input(f"lane{i}_pkt_in_vld", width=1) for i in range(4)]
    lane_in_data = [m.input(f"lane{i}_pkt_in_data", width=128) for i in range(4)]
    _lane_in_ctrl = [m.input(f"lane{i}_pkt_in_ctrl", width=5) for i in range(4)]

    fwded_vld = [m.input(f"fwded{e}_pkt_data_vld", width=1) for e in range(total_eng)]
    fwded_data = [m.input(f"fwded{e}_pkt_data", width=128) for e in range(total_eng)]

    zero1 = u(1, 0)
    zero2 = u(2, 0)
    zero128 = u(128, 0)

    m.output("pkt_in_bkpr", zero1)

    for i in range(4):
        m.output(f"lane{i}_pkt_out_vld", lane_in_vld[i])
        m.output(f"lane{i}_pkt_out_data", lane_in_data[i])

    for e in range(total_eng):
        m.output(f"fwd{e}_pkt_data_vld", fwded_vld[e])
        m.output(f"fwd{e}_pkt_data", fwded_data[e])
        m.output(f"fwd{e}_pkt_lat", zero2)
        m.output(f"fwd{e}_pkt_dp_vld", zero1)
        m.output(f"fwd{e}_pkt_dp_data", zero128)


build.__pycircuit_name__ = "fastfwd"
