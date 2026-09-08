# ndf: node=ndf://davincioo/DAV-MEM-NOC-XBAR-0001
"""Typed transport-only crossbar for the memory NOC pilot."""

import agentic_circuit as ac


@ac.struct
class NoCPacket:
    sequence: ac.u16
    transaction_id: ac.u16
    transaction_generation: ac.u16
    request_id: ac.u16
    beat_id: ac.u16
    beat_count: ac.u16
    memory_agent: ac.u8
    thread_id: ac.u16
    block_id: ac.u16
    instruction_id: ac.u16
    ordering_tag: ac.u16
    source_port: ac.u8
    destination_port: ac.u8
    is_response: bool
    address: ac.u64
    operation: ac.u8
    byte_mask: ac.u64
    return_target: ac.u8
    data_slot: ac.u16
    mrob_ref: ac.u16
    tile_id: ac.u16
    element_begin: ac.u32
    element_count: ac.u16
    fault_status: ac.u8
    payload: ac.u64
    delivered_port: ac.u8
    transport_completed: bool


@ac.system
def mem_noc_xbar_system(
    ingress_0: NoCPacket,
    ingress_1: NoCPacket,
    ingress_2: NoCPacket,
    ingress_3: NoCPacket,
) -> NoCPacket:
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
        key=lambda packet: packet.destination_port,
        depth=1,
        latency=1,
    )
    egress = ac.map({0: egress_0, 1: egress_1, 2: egress_2, 3: egress_3})
    delivered = ac.array(
        4,
        lambda port: egress[port].apply(
            lambda packet: packet.with_fields(
                delivered_port=port,
                transport_completed=True,
            ),
            depth=1,
            latency=(1, 2, 3, 5)[port],
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
