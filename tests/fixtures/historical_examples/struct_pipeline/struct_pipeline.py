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


def update_item(item: Item) -> Item:
    result = item
    result.value = item.value + 1
    result.remaining = item.remaining - 1
    return result


@module
def StructPipeline(valid: ac.u1, data: Item, take: ac.u1) -> StructResult:  # noqa: N802
    ingress = ac.queue[Item](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[Item](
        depth=2, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @rule
    def transfer() -> None:
        item = ingress.pop()
        updated = update_item(item)
        egress.push(updated)

    transfer()
    return StructResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
