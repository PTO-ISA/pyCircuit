"""Measure each byte while preserving two complete-token queue stages."""
# ruff: noqa: F821, N802 -- immutable forward queue wires, hardware root name.
import pycircuit as ac


@ac.struct
class Item:
    value: ac.u8
    priority_index: ac.u3
    high_index: ac.u3
    priority_valid: ac.u1
    # Explicit one-bit storage for the historical Boolean field(s).
    onehot_conflict: ac.u1
    population: ac.u4
    leading: ac.u4
    trailing: ac.u4


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: Item


@ac.rule
def measure(item: Item) -> Item:
    priority_index, priority_valid, onehot_conflict = ac.onehot_encode(item.value)
    high_index, _high_valid = ac.priority_encode(item.value, order="high")
    result = item
    result.priority_index = priority_index
    result.high_index = high_index
    result.priority_valid = priority_valid
    # The field boundary creates fresh fixed u1 storage; producer stays Boolean.
    result.onehot_conflict = onehot_conflict
    result.population = ac.popcount(item.value)
    result.leading = ac.count_leading_zeros(item.value)
    result.trailing = ac.count_trailing_zeros(item.value)
    return result


@ac.module
def BitPrimitivePipeline(valid: ac.u1, data: Item, take: ac.u1) -> Result:
    ready, available, incoming = ac.queue[Item](
        valid, data, measured_ready, depth=1, latency=1,
        ready_policy="downstream_pop",
    )
    measured = measure(incoming)
    measured_ready, out_valid, out_data = ac.queue[Item](
        available, measured, take, depth=1, latency=1,
        ready_policy="downstream_pop",
    )
    return Result(ready=ready, valid=out_valid, data=out_data)
