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
class Result:
    base_ready: ac.u1
    header_ready: ac.u1
    payload_ready: ac.u1
    patch_ready: ac.u1
    valid: ac.u1
    data: Packet


@ac.rule
def compose(base: Packet, header: Header, payload: Payload) -> Packet:
    return Packet(opcode=header.opcode, tag=header.tag, data=payload.data, valid=True)


@ac.rule
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
) -> Result:
    base_ready, base_available, base_value = ac.queue[Packet](
        base_valid,
        base,
        compose_ready & header_available & payload_available,  # noqa: F821
        depth=1,
        ready_policy="downstream_pop",
    )
    header_ready, header_available, header_value = ac.queue[Header](
        header_valid,
        header,
        compose_ready & base_available & payload_available,  # noqa: F821
        depth=1,
        ready_policy="downstream_pop",
    )
    payload_ready, payload_available, payload_value = ac.queue[Payload](
        payload_valid,
        payload,
        compose_ready & base_available & header_available,  # noqa: F821
        depth=1,
        ready_policy="downstream_pop",
    )
    patch_ready, patch_available, patch_value = ac.queue[Patch](
        patch_valid,
        patch,
        update_ready & composed_available,  # noqa: F821
        depth=1,
        ready_policy="downstream_pop",
    )
    composed = compose(base_value, header_value, payload_value)
    compose_ready, composed_available, composed_value = ac.queue[Packet](
        base_available & header_available & payload_available,
        composed,
        update_ready & patch_available,  # noqa: F821
        depth=1,
        ready_policy="downstream_pop",
    )
    updated = apply_patch(composed_value, patch_value)
    update_ready, out_valid, out_data = ac.queue[Packet](
        composed_available & patch_available,
        updated,
        take,
        depth=1,
        ready_policy="downstream_pop",
    )
    return Result(
        base_ready=base_ready,
        header_ready=header_ready,
        payload_ready=payload_ready,
        patch_ready=patch_ready,
        valid=out_valid,
        data=out_data,
    )
