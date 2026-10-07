"""Two depth-two queues surrounding independent unsigned64-bit field updates."""

import pycircuit as ac
from pycircuit import module, rule


@ac.struct
class Item:
    value: ac.u64
    remaining: ac.u64


@ac.struct
class StructResult:
    ready: ac.u1
    valid: ac.u1
    data: Item


@rule
def update_item(item: Item) -> Item:
    result = item
    result.value = item.value + 1
    result.remaining = item.remaining - 1
    return result


@module
def StructPipeline(  # noqa: N802
    valid: ac.u1, data: Item, take: ac.u1
) -> StructResult:
    ready, available, item = ac.queue[Item](
        valid,
        data,
        stage_ready,  # noqa: F821
        depth=2,
        ready_policy="downstream_pop",
    )
    updated = update_item(item)
    stage_ready, out_valid, out_data = ac.queue[Item](
        available, updated, take, depth=2, ready_policy="downstream_pop"
    )
    return StructResult(ready=ready, valid=out_valid, data=out_data)
