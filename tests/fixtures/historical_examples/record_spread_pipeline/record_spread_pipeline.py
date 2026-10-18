"""Join independent record streams and patch fields without changing timing."""

import pycircuit as ac


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
    valid: ac.u1


@ac.struct
class Patch:
    tag: ac.u4
    valid: ac.u1


@ac.struct
class PacketChannel:
    valid: ac.u1
    data: Packet


def compose(base: Packet, header: Header, payload: Payload) -> Packet:
    return Packet(opcode=header.opcode, tag=header.tag, data=payload.data, valid=True)


def apply_patch(packet: Packet, patch: Patch) -> Packet:
    result = packet
    result.tag = patch.tag
    result.valid = patch.valid
    return result


@ac.module
def RecordSpreadPipeline(  # noqa: N802
    base_valid: ac.u1,
    base: Packet,
    header_valid: ac.u1,
    header: Header,
    payload_valid: ac.u1,
    payload: Payload,
    patch_valid: ac.u1,
    patch: Patch,
    take: ac.u1,
) -> tuple[ac.u1, ac.u1, ac.u1, ac.u1, PacketChannel]:
    base_tokens = ac.queue[Packet](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=base_valid,
        in_data=base,
    )
    header_tokens = ac.queue[Header](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=header_valid,
        in_data=header,
    )
    payload_tokens = ac.queue[Payload](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=payload_valid,
        in_data=payload,
    )
    patch_tokens = ac.queue[Patch](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=patch_valid,
        in_data=patch,
    )
    composed_tokens = ac.queue[Packet](
        depth=1, latency=1, ready_policy="downstream_pop"
    )
    updated_tokens = ac.queue[Packet](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def compose_packet() -> None:
        base_token = base_tokens.pop()
        header_token = header_tokens.pop()
        payload_token = payload_tokens.pop()
        composed_tokens.push(compose(base_token, header_token, payload_token))

    @ac.rule
    def patch_packet() -> None:
        packet = composed_tokens.pop()
        patch_token = patch_tokens.pop()
        updated_tokens.push(apply_patch(packet, patch_token))

    compose_packet()
    patch_packet()
    return (
        base_tokens.ready,
        header_tokens.ready,
        payload_tokens.ready,
        patch_tokens.ready,
        PacketChannel(valid=updated_tokens.valid, data=updated_tokens.data),
    )
