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


def count_item(item: Item) -> Item:
    result = item
    result.count = ac.popcount(item.value)
    return result


@ac.module
def PopcountPipeline(valid: ac.u1, data: Item, take: ac.u1) -> Result:  # noqa: N802
    ingress = ac.queue[Item](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[Item](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        item = ingress.pop()
        counted = count_item(item)
        egress.push(counted)

    transfer()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)
