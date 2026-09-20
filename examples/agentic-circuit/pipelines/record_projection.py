"""Explicit exact-name projection into a smaller nominal record."""

import agentic_circuit as ac


@ac.struct
class Header:
    opcode: ac.u4


@ac.struct
class Packet:
    header: Header
    tag: ac.u8
    payload: ac.u16
    valid: bool


@ac.struct
class HeaderView:
    valid: bool
    header: Header


@ac.rule
def project(packet: Packet) -> HeaderView:
    return packet.project(HeaderView)


@ac.rule
def update(packet: Packet) -> Packet:
    thin = packet.project(HeaderView)
    enabled = thin.with_fields(valid=True)
    return packet.with_fields(**enabled)


@ac.system
def record_projection(packet: Packet) -> HeaderView:
    result = project(packet)
    return result


@ac.system
def record_projection_update(packet: Packet) -> Packet:
    result = update(packet)
    return result
