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


def advance(item: Packet) -> Packet:
    result = item
    result.header.mode = item.header.mode + 1
    return result


@ac.module
def NestedPayloadPipeline(  # noqa: N802
    valid: ac.u1, data: Packet, take: ac.u1
) -> AdvanceResult:
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
        item = ingress.pop()
        advanced = advance(item)
        egress.push(advanced)

    transfer()
    return AdvanceResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
