# ndf: node=ndf://davincioo/DAV-TMU-BGF-XBAR-0001
"""Typed four-requester routing fabric for the TMU block-group fabric."""

import agentic_circuit as ac


@ac.struct
class BGFPacket:
    sequence: ac.u16
    requester: ac.u8
    thread_id: ac.u16
    block_id: ac.u16
    operation_id: ac.u16
    tile_id: ac.u16
    allocation_generation: ac.u16
    destination_bank: ac.u8
    element_begin: ac.u32
    element_count: ac.u16
    byte_mask: ac.u64
    is_write: bool
    ordering_tag: ac.u16
    definedness_tag: ac.u16
    payload: ac.u64
    delivered_bank: ac.u8
    completed: bool


@ac.system
def bgf_xbar_system(
    ingress_0: BGFPacket,
    ingress_1: BGFPacket,
    ingress_2: BGFPacket,
    ingress_3: BGFPacket,
) -> BGFPacket:
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
        key=lambda packet: packet.destination_bank,
        depth=1,
        latency=1,
    )
    egress = ac.map({0: egress_0, 1: egress_1, 2: egress_2, 3: egress_3})
    delivered = ac.array(
        4,
        lambda bank: egress[bank].apply(
            lambda packet: packet.with_fields(
                delivered_bank=bank,
                completed=True,
            ),
            depth=1,
            latency=(1, 2, 4, 7)[bank],
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
