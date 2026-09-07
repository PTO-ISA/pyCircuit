"""Stateless typed record construction for ACIR-to-PYC parity."""

import agentic_circuit as ac


@ac.struct
class Packet:
    tag: ac.u3
    value: ac.u8
    valid: bool


@ac.rule
def update_packet(packet: Packet) -> Packet:
    return Packet(
        tag=packet.tag,
        value=packet.value + 1,
        valid=not packet.valid,
    )


@ac.system
def record_transform(packet: Packet) -> Packet:
    updated = update_packet(packet)
    return updated
