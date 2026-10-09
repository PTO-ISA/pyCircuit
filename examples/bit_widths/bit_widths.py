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


@ac.rule
def transform(item: MaskedTag) -> MaskedTag:
    result = item
    result.value = (item.value & item.mask) ^ 1
    result.rotated = (item.value << 1) | (item.value >> 12)
    return result


@ac.module
def BitWidths(valid: ac.u1, data: MaskedTag, take: ac.u1) -> Result:  # noqa: N802
    ready, available, head = ac.queue[MaskedTag](
        valid, data, stage_ready, depth=1, ready_policy="downstream_pop"  # noqa: F821
    )
    transformed = transform(head)
    stage_ready, out_valid, out_data = ac.queue[MaskedTag](
        available, transformed, take, depth=1, ready_policy="downstream_pop"
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
