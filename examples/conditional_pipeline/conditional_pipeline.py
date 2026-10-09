"""Selected-route buffering with false-branch priority at the merge."""

# ruff: noqa: F821, N802 -- ordinary forward hardware queue-result wires.
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
    ready, available, value = ac.queue[Item](
        valid,
        data,
        true_fan_ready if value.route == 0 else false_fan_ready,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    is_true = value.route == 0
    false_fan_ready, false_fan_valid, false_fan_data = ac.queue[Item](
        available and not is_true,
        value,
        false_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    true_fan_ready, true_fan_valid, true_fan_data = ac.queue[Item](
        available and is_true,
        value,
        true_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    false_value = Item(value=false_fan_data.value + 20, route=false_fan_data.route)
    true_value = Item(value=true_fan_data.value + 10, route=true_fan_data.route)
    false_ready, false_valid, false_data = ac.queue[Item](
        false_fan_valid,
        false_value,
        merged_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    true_ready, true_valid, true_data = ac.queue[Item](
        true_fan_valid,
        true_value,
        merged_ready and not false_valid,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    merged_ready, out_valid, out_data = ac.queue[Item](
        false_valid or true_valid,
        false_data if false_valid else true_data,
        take,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    return ConditionalResult(ready=ready, valid=out_valid, data=out_data)
