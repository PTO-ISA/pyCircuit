from __future__ import annotations

from pycircuit.dsl import Signal
from pycircuit.hw import Circuit, ClockDomain, Wire
from pycircuit.literals import u


class RegFileError(ValueError):
    """Invalid RegFile port wiring."""


def RegFile(
    m: Circuit,
    cd: ClockDomain,
    raddr_bus: Wire,
    wen_bus: Wire,
    waddr_bus: Wire,
    wdata_bus: Wire,
    *,
    ptag_count: int = 256,
    const_count: int = 128,
    nr: int = 10,
    nw: int = 5,
):
    ptag_n = int(ptag_count)
    const_n = int(const_count)
    nr_n = int(nr)
    nw_n = int(nw)
    if ptag_n <= 0:
        raise ValueError("RegFile ptag_count must be > 0")
    if const_n < 0 or const_n > ptag_n:
        raise ValueError(
            "RegFile const_count must satisfy 0 <= const_count <= ptag_count"
        )
    if nr_n <= 0:
        raise ValueError("RegFile nr must be > 0")
    if nw_n <= 0:
        raise ValueError("RegFile nw must be > 0")
    ptag_w = max(1, (ptag_n - 1).bit_length())

    clk_v = cd.clk
    rst_v = cd.rst
    if not isinstance(clk_v, Signal) or clk_v.ty != "!pyc.clock":
        raise RegFileError("RegFile domain clk must be !pyc.clock")
    if not isinstance(rst_v, Signal) or rst_v.ty != "!pyc.reset":
        raise RegFileError("RegFile domain rst must be !pyc.reset")

    raddr_bus_w = raddr_bus
    wen_bus_w = wen_bus
    waddr_bus_w = waddr_bus
    wdata_bus_w = wdata_bus

    exp_raddr_w = nr_n * ptag_w
    exp_wen_w = nw_n
    exp_waddr_w = nw_n * ptag_w
    exp_wdata_w = nw_n * 64

    if raddr_bus_w.width != exp_raddr_w:
        raise RegFileError(f"RegFile.raddr_bus must be i{exp_raddr_w}")
    if wen_bus_w.width != exp_wen_w:
        raise RegFileError(f"RegFile.wen_bus must be i{exp_wen_w}")
    if waddr_bus_w.width != exp_waddr_w:
        raise RegFileError(f"RegFile.waddr_bus must be i{exp_waddr_w}")
    if wdata_bus_w.width != exp_wdata_w:
        raise RegFileError(f"RegFile.wdata_bus must be i{exp_wdata_w}")

    storage_depth = ptag_n - const_n
    bank0 = [
        m.out(f"rf_bank0_{i}", domain=cd, width=32, init=u(32, 0))
        for i in range(storage_depth)
    ]
    bank1 = [
        m.out(f"rf_bank1_{i}", domain=cd, width=32, init=u(32, 0))
        for i in range(storage_depth)
    ]

    raddr_lanes = [raddr_bus_w[i * ptag_w : (i + 1) * ptag_w] for i in range(nr_n)]
    wen_lanes = [wen_bus_w[i] for i in range(nw_n)]
    waddr_lanes = [waddr_bus_w[i * ptag_w : (i + 1) * ptag_w] for i in range(nw_n)]
    wdata_lanes = [wdata_bus_w[i * 64 : (i + 1) * 64] for i in range(nw_n)]
    wdata_lo = [w[0:32] for w in wdata_lanes]
    wdata_hi = [w[32:64] for w in wdata_lanes]

    # Multiple writes to the same storage PTAG in one cycle are intentionally
    # left undefined by contract (strict no-conflict mode).
    for sidx in range(storage_depth):
        ptag = const_n + sidx
        we_any = u(1, 0)
        next_lo = bank0[sidx].out()
        next_hi = bank1[sidx].out()
        for lane in range(nw_n):
            hit = wen_lanes[lane] & (waddr_lanes[lane] == u(ptag_w, ptag))
            we_any = we_any | hit
            next_lo = hit._select_internal(wdata_lo[lane], next_lo)
            next_hi = hit._select_internal(wdata_hi[lane], next_hi)
        bank0[sidx].set(next_lo, when=we_any)
        bank1[sidx].set(next_hi, when=we_any)

    cmp_w = ptag_w + 1
    rdata_lanes = []
    for lane in range(nr_n):
        raddr_i = raddr_lanes[lane]
        raddr_ext = raddr_i + u(cmp_w, 0)
        is_valid = raddr_ext < u(cmp_w, ptag_n)
        is_const = raddr_ext < u(cmp_w, const_n)

        if raddr_i.width > 32:
            const32 = raddr_i[0:32]
        else:
            const32 = raddr_i + u(32, 0)
        const64 = m.cat(const32, const32)

        store_lo = u(32, 0)
        store_hi = u(32, 0)
        for sidx in range(storage_depth):
            ptag = const_n + sidx
            hit = raddr_i == u(ptag_w, ptag)
            store_lo = hit._select_internal(bank0[sidx].out(), store_lo)
            store_hi = hit._select_internal(bank1[sidx].out(), store_hi)
        store64 = m.cat(store_hi, store_lo)

        lane_data = is_const._select_internal(const64, store64)
        lane_data = is_valid._select_internal(lane_data, u(64, 0))
        rdata_lanes.append(lane_data)

    rdata_bus_out = rdata_lanes[0]
    for lane in range(1, nr_n):
        rdata_bus_out = m.cat(rdata_lanes[lane], rdata_bus_out)

    return m.bundle_connector(
        rdata_bus=rdata_bus_out,
    )
