"""Project named record fields through the original two token stages."""

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
class HeaderView:
    valid: ac.u1
    header: Header


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: HeaderView


@ac.rule
def project(packet: Packet) -> HeaderView:
    return HeaderView(valid=packet.valid, header=packet.header)


@ac.module
def RecordProjection(valid: ac.u1, data: Packet, take: ac.u1) -> Result:  # noqa: N802
    ready, available, packet = ac.queue[Packet](
        valid, data, stage_ready, depth=1, ready_policy="downstream_pop"  # noqa: F821
    )
    projected = project(packet)
    stage_ready, out_valid, out_data = ac.queue[HeaderView](
        available, projected, take, depth=1, ready_policy="downstream_pop"
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
