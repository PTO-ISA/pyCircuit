"""Compose and update immutable records by exact destination field name."""

import agentic_circuit as ac


@ac.struct
class Header:
    opcode: ac.u4
    tag: ac.u4


@ac.struct
class Payload:
    data: ac.u16


@ac.struct
class Packet:
    opcode: ac.u4
    tag: ac.u4
    data: ac.u16
    valid: bool


@ac.struct
class Patch:
    tag: ac.u4
    valid: bool


@ac.rule
def compose(base: Packet, header: Header, payload: Payload) -> Packet:
    return Packet(**header, **payload, valid=True)


@ac.rule
def apply_patch(packet: Packet, patch: Patch) -> Packet:
    return packet.with_fields(**patch)


@ac.system
def record_spread_pipeline(
    base: Packet,
    header: Header,
    payload: Payload,
    patch: Patch,
) -> Packet:
    composed = compose(base, header, payload)
    updated = apply_patch(composed, patch)
    return updated


specialization = ac.jit(record_spread_pipeline)
