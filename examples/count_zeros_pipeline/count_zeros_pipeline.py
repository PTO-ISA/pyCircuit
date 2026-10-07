"""Preserve a 13-bit payload while replacing both endpoint-zero counts."""
# ruff: noqa: F821, N802 -- immutable forward queue result wires.
import pycircuit as ac


@ac.struct
class Item:
    value: ac.u13
    leading: ac.u4
    trailing: ac.u4


@ac.struct
class CountZerosResult:
    ready: ac.u1
    valid: ac.u1
    data: Item


@ac.module
def CountZerosPipeline(valid: ac.u1, data: Item, take: ac.u1) -> CountZerosResult:
    ready, available, head = ac.queue[Item](
        valid, data, counted_ready, depth=1, latency=1,
        ready_policy="downstream_pop",
    )
    counted = Item(
        value=head.value,
        leading=ac.count_leading_zeros(head.value),
        trailing=ac.count_trailing_zeros(head.value),
    )
    counted_ready, out_valid, out_data = ac.queue[Item](
        available, counted, take, depth=1, latency=1,
        ready_policy="downstream_pop",
    )
    return CountZerosResult(ready=ready, valid=out_valid, data=out_data)
