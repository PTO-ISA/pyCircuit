"""Set the payload valid bit while retaining the original token pipeline."""

import pycircuit as ac


@ac.struct
class Header:
    opcode: ac.u4


@ac.struct
class Packet:
    header: Header
    tag: ac.u8
    payload: ac.u16
    valid: ac.u1


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: Packet


@ac.rule
def update(packet: Packet) -> Packet:
    result = packet
    result.valid = True
    return result


@ac.module
def RecordProjectionUpdate(  # noqa: N802 - hardware module definition
    valid: ac.u1, data: Packet, take: ac.u1
) -> Result:  # noqa: N802
    ready, available, packet = ac.queue[Packet](
        valid, data, stage_ready, depth=1, ready_policy="downstream_pop"  # noqa: F821
    )
    updated = update(packet)
    stage_ready, out_valid, out_data = ac.queue[Packet](
        available, updated, take, depth=1, ready_policy="downstream_pop"
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
