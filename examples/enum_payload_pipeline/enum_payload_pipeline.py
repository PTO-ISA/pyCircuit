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


@ac.rule
def classify(item: Packet) -> Packet:
    result = item
    result.header.mode = Mode.RUN
    result.matched = item.header.mode == Mode.WAIT
    return result


@ac.module
def EnumPayloadPipeline(  # noqa: N802
    valid: ac.u1, data: Packet, take: ac.u1
) -> Result:
    ready, available, packet = ac.queue[Packet](
        valid, data, stage_ready, depth=1, ready_policy="downstream_pop"  # noqa: F821
    )
    classified = classify(packet)
    stage_ready, out_valid, out_data = ac.queue[Packet](
        available, classified, take, depth=1, ready_policy="downstream_pop"
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
