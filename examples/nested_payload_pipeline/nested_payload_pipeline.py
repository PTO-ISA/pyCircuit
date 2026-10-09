"""Advance the nested mode while preserving opcode and payload."""

import pycircuit as ac


@ac.struct
class Header:
    opcode: ac.u6
    mode: ac.u3


@ac.struct
class Packet:
    header: Header
    payload: ac.u17


@ac.struct
class AdvanceResult:
    ready: ac.u1
    valid: ac.u1
    data: Packet


@ac.rule
def advance(item: Packet) -> Packet:
    result = item
    result.header.mode = item.header.mode + 1
    return result


@ac.module
def NestedPayloadPipeline(  # noqa: N802
    valid: ac.u1, data: Packet, take: ac.u1
) -> AdvanceResult:
    ready, available, item = ac.queue[Packet](
        valid, data, stage_ready, depth=1, ready_policy="downstream_pop"  # noqa: F821
    )
    advanced = advance(item)
    stage_ready, out_valid, out_data = ac.queue[Packet](
        available, advanced, take, depth=1, ready_policy="downstream_pop"
    )
    return AdvanceResult(ready=ready, valid=out_valid, data=out_data)
