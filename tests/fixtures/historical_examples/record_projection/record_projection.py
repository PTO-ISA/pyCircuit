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


def project(packet: Packet) -> HeaderView:
    return HeaderView(valid=packet.valid, header=packet.header)


@ac.module
def RecordProjection(valid: ac.u1, data: Packet, take: ac.u1) -> Result:  # noqa: N802
    ingress = ac.queue[Packet](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[HeaderView](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        packet = ingress.pop()
        projected = project(packet)
        egress.push(projected)

    transfer()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)
