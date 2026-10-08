"""Two one-token queue stages preserving a value and replacing its count."""

import pycircuit as ac


@ac.struct
class Item:
    value: ac.u13
    count: ac.u4


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: Item


@ac.rule
def count_item(item: Item) -> Item:
    result = item
    result.count = ac.popcount(item.value)
    return result


@ac.module
def PopcountPipeline(valid: ac.u1, data: Item, take: ac.u1) -> Result:  # noqa: N802
    ready, available, item = ac.queue[Item](
        valid,
        data,
        stage_ready,  # noqa: F821
        depth=1,
        ready_policy="downstream_pop",
    )
    counted = count_item(item)
    stage_ready, out_valid, out_data = ac.queue[Item](
        available, counted, take, depth=1, ready_policy="downstream_pop"
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
