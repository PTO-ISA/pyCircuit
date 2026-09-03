from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    cas,
    mux,
    u,
    wire_of,
)


def build(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    ptag_count: int = 256,
    const_count: int = 128,
    nr: int = 10,
    nw: int = 5,
) -> None:
    ptag_n = int(ptag_count)
    const_n = int(const_count)
    nr_n = int(nr)
    nw_n = int(nw)
    if ptag_n <= 0:
        raise ValueError("regfile ptag_count must be > 0")
    if const_n < 0 or const_n > ptag_n:
        raise ValueError(
            "regfile const_count must satisfy 0 <= const_count <= ptag_count"
        )
    if nr_n <= 0:
        raise ValueError("regfile nr must be > 0")
    if nw_n <= 0:
        raise ValueError("regfile nw must be > 0")

    ptag_w = max(1, (ptag_n - 1).bit_length())
    storage_depth = ptag_n - const_n
    cmp_w = ptag_w + 1

    # ══════════════════════════════════════════════════════════════
    # Cycle 0 — Inputs
    # ══════════════════════════════════════════════════════════════
    raddr = [
        cas(domain, m.input(f"raddr{i}", width=ptag_w), cycle=0) for i in range(nr_n)
    ]
    wen = [cas(domain, m.input(f"wen{i}", width=1), cycle=0) for i in range(nw_n)]
    waddr = [
        cas(domain, m.input(f"waddr{i}", width=ptag_w), cycle=0) for i in range(nw_n)
    ]
    wdata = [cas(domain, m.input(f"wdata{i}", width=64), cycle=0) for i in range(nw_n)]

    wdata_lo = [wd[0:32] for wd in wdata]
    wdata_hi = [wd[32:64] for wd in wdata]

    # ══════════════════════════════════════════════════════════════
    # Cycle 0 — Storage state (feedback registers via domain.signal)
    # ══════════════════════════════════════════════════════════════
    bank0 = [
        domain.signal(width=32, reset_value=0, name=f"rf_bank0_{i}")
        for i in range(storage_depth)
    ]
    bank1 = [
        domain.signal(width=32, reset_value=0, name=f"rf_bank1_{i}")
        for i in range(storage_depth)
    ]

    # ══════════════════════════════════════════════════════════════
    # Cycle 0 — Combinational read logic
    # ══════════════════════════════════════════════════════════════
    zero32 = cas(domain, m.const(0, width=32), cycle=0)
    zero64 = cas(domain, m.const(0, width=64), cycle=0)

    for i in range(nr_n):
        ra = raddr[i]
        ra_ext = cas(domain, wire_of(ra) + u(cmp_w, 0), cycle=ra.cycle)
        is_valid = ra_ext < cas(domain, m.const(ptag_n, width=cmp_w), cycle=0)
        is_const = ra_ext < cas(domain, m.const(const_n, width=cmp_w), cycle=0)

        if ptag_w > 32:
            const32 = ra[0:32]
        else:
            const32 = cas(domain, wire_of(ra) + u(32, 0), cycle=ra.cycle)
        const64 = cas(
            domain,
            m.cat(wire_of(const32), wire_of(const32)),
            cycle=const32.cycle,
        )

        store_lo: CycleAwareSignal = zero32
        store_hi: CycleAwareSignal = zero32
        for sidx in range(storage_depth):
            ptag = const_n + sidx
            hit = ra == cas(domain, m.const(ptag, width=ptag_w), cycle=0)
            store_lo = mux(hit, bank0[sidx], store_lo)
            store_hi = mux(hit, bank1[sidx], store_hi)
        store64 = cas(
            domain,
            m.cat(wire_of(store_hi), wire_of(store_lo)),
            cycle=store_hi.cycle,
        )

        lane_data = mux(is_const, const64, store64)
        lane_data = mux(is_valid, lane_data, zero64)
        m.output(f"rdata{i}", wire_of(lane_data))

    # ══════════════════════════════════════════════════════════════
    # domain.next() → Cycle 1 — Synchronous write (close feedback)
    # ══════════════════════════════════════════════════════════════
    domain.next()

    for sidx in range(storage_depth):
        ptag = const_n + sidx
        we_any = cas(domain, m.const(0, width=1), cycle=0)
        next_lo: CycleAwareSignal = bank0[sidx]
        next_hi: CycleAwareSignal = bank1[sidx]
        for lane in range(nw_n):
            hit = wen[lane] & (
                waddr[lane] == cas(domain, m.const(ptag, width=ptag_w), cycle=0)
            )
            we_any = we_any | hit
            next_lo = mux(hit, wdata_lo[lane], next_lo)
            next_hi = mux(hit, wdata_hi[lane], next_hi)
        bank0[sidx].assign(next_lo, when=we_any)
        bank1[sidx].assign(next_hi, when=we_any)


build.__pycircuit_name__ = "regfile"


if __name__ == "__main__":
    pass
