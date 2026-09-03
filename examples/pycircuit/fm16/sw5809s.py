"""Simplified SW5809s switch — pyCircuit V6 cycle-aware."""

from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    cas,
    compile_cycle_aware,
    mux,
    u,
)

PKT_W = 32


def build(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    N_PORTS: int = 4,
    VOQ_DEPTH: int = 4,
) -> None:
    cd = domain.clock_domain

    PORT_BITS = max((N_PORTS - 1).bit_length(), 1)

    in_pkts = [m.input(f"in_pkt_{i}", width=PKT_W) for i in range(N_PORTS)]
    in_vals = [m.input(f"in_valid_{i}", width=1) for i in range(N_PORTS)]

    voqs = []
    for i in range(N_PORTS):
        row = []
        for j in range(N_PORTS):
            q = m.rv_queue(f"voq_{i}_{j}", domain=cd, width=PKT_W, depth=VOQ_DEPTH)
            row.append(q)
        voqs.append(row)

    for i in range(N_PORTS):
        pkt_dst = in_pkts[i][24:28][0:PORT_BITS]
        for j in range(N_PORTS):
            dst_match = (pkt_dst == u(PORT_BITS, j)) & in_vals[i]
            voqs[i][j].push(in_pkts[i], when=dst_match)

    rr_states = [
        domain.signal(width=PORT_BITS, reset_value=0, name=f"rr_{j}")
        for j in range(N_PORTS)
    ]

    out_pkts = []
    out_vals = []

    for j in range(N_PORTS):
        peeks = []
        for i in range(N_PORTS):
            peek = voqs[i][j].pop(when=u(1, 0))
            peeks.append(peek)

        sel_pkt = u(PKT_W, 0)
        sel_val = u(1, 0)

        for i in range(N_PORTS):
            has_data = peeks[i].valid
            sel_pkt = mux(has_data, peeks[i].data, sel_pkt)
            sel_val = has_data | sel_val

        out_pkts.append(sel_pkt)
        out_vals.append(sel_val)

    domain.next()

    for j in range(N_PORTS):
        rr_cur = rr_states[j]
        # Constants after next() need cycle=0 cas to avoid unintended _v6_bal.
        wrap = rr_cur == cas(domain, u(PORT_BITS, N_PORTS - 1), cycle=0)
        next_rr = mux(wrap, cas(domain, u(PORT_BITS, 0), cycle=0), rr_cur + 1)
        rr_states[j].assign(next_rr, when=cas(domain, out_vals[j], cycle=0))

    for j in range(N_PORTS):
        m.output(f"out_pkt_{j}", out_pkts[j])
        m.output(f"out_valid_{j}", out_vals[j])


build.__pycircuit_name__ = "sw5809s"

if __name__ == "__main__":
    circuit = compile_cycle_aware(
        build, name="sw5809s", eager=True, N_PORTS=4, VOQ_DEPTH=4
    )
    print(circuit.emit_mlir()[:500])
    print(f"... ({len(circuit.emit_mlir())} chars)")
