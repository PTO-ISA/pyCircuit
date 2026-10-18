"""Two one-token stages carrying the original exact-width record transform."""

import pycircuit as ac


@ac.struct
class MaskedTag:
    value: ac.u13
    mask: ac.u13
    rotated: ac.u13
    sequence: ac.u37


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: MaskedTag


def transform(item: MaskedTag) -> MaskedTag:
    result = item
    result.value = (item.value & item.mask) ^ 1
    result.rotated = (item.value << 1) | (item.value >> 12)
    return result


@ac.module
def BitWidths(valid: ac.u1, data: MaskedTag, take: ac.u1) -> Result:  # noqa: N802
    ingress = ac.queue[MaskedTag](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[MaskedTag](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        head = ingress.pop()
        transformed = transform(head)
        egress.push(transformed)

    transfer()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)
