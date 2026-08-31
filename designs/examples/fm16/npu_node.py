"""Simplified NPU node — pyCircuit V6 cycle-aware."""

from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
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
    FIFO_DEPTH: int = 8,
    NODE_ID: int = 0,
) -> None:
    cd = domain.clock_domain

    hbm_pkt = m.input("hbm_pkt", width=PKT_W)
    hbm_valid = m.input("hbm_valid", width=1)

    rx_pkts = [m.input(f"rx_pkt_{i}", width=PKT_W) for i in range(N_PORTS)]
    rx_vals = [m.input(f"rx_valid_{i}", width=1) for i in range(N_PORTS)]

    fifos = []
    for i in range(N_PORTS):
        q = m.rv_queue(f"oq_{i}", domain=cd, width=PKT_W, depth=FIFO_DEPTH)
        fifos.append(q)

    PORT_BITS = max((N_PORTS - 1).bit_length(), 1)
    hbm_dst = hbm_pkt[24:28]
    hbm_port = hbm_dst[0:PORT_BITS]

    for j in range(N_PORTS):
        merged_data = u(PKT_W, 0)
        merged_valid = u(1, 0)

        for i in range(N_PORTS):
            rx_dst_i = rx_pkts[i][24:28]
            rx_port_i = rx_dst_i[0:PORT_BITS]
            fwd_match = (rx_port_i == u(PORT_BITS, j)) & rx_vals[i]
            merged_data = mux(fwd_match, rx_pkts[i], merged_data)
            merged_valid = fwd_match | merged_valid

        hbm_match_j = hbm_valid & (hbm_port == u(PORT_BITS, j))
        merged_data = mux(hbm_match_j, hbm_pkt, merged_data)
        merged_valid = hbm_match_j | merged_valid

        fifos[j].push(merged_data, when=merged_valid)

    tx_pkts = []
    tx_vals = []
    for i in range(N_PORTS):
        pop_result = fifos[i].pop(when=u(1, 1))
        tx_pkts.append(pop_result.data)
        tx_vals.append(pop_result.valid)

    hbm_ready_sig = u(1, 1)

    for i in range(N_PORTS):
        m.output(f"tx_pkt_{i}", tx_pkts[i])
        m.output(f"tx_valid_{i}", tx_vals[i])
    m.output("hbm_ready", hbm_ready_sig)


build.__pycircuit_name__ = "npu_node"

if __name__ == "__main__":
    circuit = compile_cycle_aware(
        build, name="npu_node", eager=True, N_PORTS=4, FIFO_DEPTH=8, NODE_ID=0
    )
    print(circuit.emit_mlir()[:500])
    print(f"... ({len(circuit.emit_mlir())} chars)")
