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


def update(packet: Packet) -> Packet:
    result = packet
    result.valid = True
    return result


@ac.module
def RecordProjectionUpdate(  # noqa: N802 - hardware module definition
    valid: ac.u1, data: Packet, take: ac.u1
) -> Result:  # noqa: N802
    ingress = ac.queue[Packet](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[Packet](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        packet = ingress.pop()
        updated = update(packet)
        egress.push(updated)

    transfer()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)
