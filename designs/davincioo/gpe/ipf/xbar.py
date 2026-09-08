# ndf: node=ndf://davincioo/DAV-GPE-IPF-XBAR-0001
"""Typed four-PE transport fabric for grouped processing."""

import agentic_circuit as ac


@ac.struct
class GPEPacket:
    sequence: ac.u16
    source_pe: ac.u8
    destination_pe: ac.u8
    thread_id: ac.u16
    block_id: ac.u16
    operation_id: ac.u16
    group_id: ac.u16
    participant_mask: ac.u64
    producer_generation: ac.u16
    token_kind: ac.u8
    ordering_tag: ac.u16
    payload: ac.u64
    delivered_pe: ac.u8
    completed: bool


@ac.system
def gpe_ipf_xbar_system(
    ingress_0: GPEPacket,
    ingress_1: GPEPacket,
    ingress_2: GPEPacket,
    ingress_3: GPEPacket,
) -> GPEPacket:
    arbitrated = ingress_0.merge(
        ingress_1,
        ingress_2,
        ingress_3,
        policy="round_robin",
        depth=2,
        latency=1,
    )
    egress_0, egress_1, egress_2, egress_3 = arbitrated.route(
        outputs=4,
        key=lambda packet: packet.destination_pe,
        depth=1,
        latency=1,
    )
    egress = ac.map({0: egress_0, 1: egress_1, 2: egress_2, 3: egress_3})
    delivered = ac.array(
        4,
        lambda pe: egress[pe].apply(
            lambda packet: packet.with_fields(delivered_pe=pe, completed=True),
            depth=1,
            latency=(1, 2, 3, 4)[pe],
        ),
    )
    completed = delivered[0].merge(
        delivered[1],
        delivered[2],
        delivered[3],
        policy="round_robin",
        depth=2,
        latency=1,
    )
    return completed
