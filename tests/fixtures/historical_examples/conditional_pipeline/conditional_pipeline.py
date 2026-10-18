"""Selected-route buffering with false-branch priority at the merge."""

# ruff: noqa: N802 -- hardware module name.
import pycircuit as ac


@ac.struct
class Item:
    value: ac.u32
    route: ac.u1


@ac.struct
class ConditionalResult:
    ready: ac.u1
    valid: ac.u1
    data: Item


@ac.module
def ConditionalPipeline(valid: ac.u1, data: Item, take: ac.u1) -> ConditionalResult:
    ingress = ac.queue[Item](depth=2, latency=1, in_valid=valid, in_data=data)
    false_fan = ac.queue[Item](depth=1, latency=1)
    true_fan = ac.queue[Item](depth=1, latency=1)
    false_result = ac.queue[Item](depth=1, latency=1)
    true_result = ac.queue[Item](depth=1, latency=1)
    merged = ac.queue[Item](depth=1, latency=1, out_ready=take)

    @ac.rule
    def route() -> None:
        item = ingress.pop()
        if item.route == 0:
            true_fan.push(item)
        else:
            false_fan.push(item)

    @ac.rule
    def transform_false() -> None:
        item = false_fan.pop()
        false_result.push(Item(value=item.value + 20, route=item.route))

    @ac.rule
    def transform_true() -> None:
        item = true_fan.pop()
        true_result.push(Item(value=item.value + 10, route=item.route))

    @ac.rule
    def merge() -> None:
        # This is the design's arbitration policy: false has priority.
        if false_result.valid:
            item = false_result.pop()
        else:
            item = true_result.pop()
        merged.push(item)

    route()
    transform_false()
    transform_true()
    merge()
    return ConditionalResult(ready=ingress.ready, valid=merged.valid, data=merged.data)
