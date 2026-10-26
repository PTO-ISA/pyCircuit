"""Update a nested enum while comparing the original input value."""

from enum import Enum

import pycircuit as ac


@ac.encoding(width=2)
class Mode(Enum):
    IDLE = 0
    RUN = 1
    WAIT = 2


@ac.struct
class Header:
    opcode: ac.u6
    mode: Mode


@ac.struct
class Packet:
    header: Header
    payload: ac.u17
    matched: ac.u1


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: Packet


def classify(item: Packet) -> Packet:
    result = item
    result.header.mode = Mode.RUN
    result.matched = item.header.mode == Mode.WAIT
    return result


@ac.module
def EnumPayloadPipeline(  # noqa: N802
    valid: ac.u1, data: Packet, take: ac.u1
) -> Result:
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
        classified = classify(packet)
        egress.push(classified)

    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)
