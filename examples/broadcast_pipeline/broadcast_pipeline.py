"""Atomic unsigned64 broadcast, two transforms and left-priority merge."""

# ruff: noqa: F821, N802 -- ordinary forward hardware queue-result wires.
import pycircuit as ac


@ac.struct
class BroadcastResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u64


@ac.module
def BroadcastPipeline(valid: ac.u1, data: ac.u64, take: ac.u1) -> BroadcastResult:
    ready, available, value = ac.queue[ac.u64](
        valid,
        data,
        left_fan_ready & right_fan_ready,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    left_fan_ready, left_fan_valid, left_fan_data = ac.queue[ac.u64](
        available & left_fan_ready & right_fan_ready,
        value,
        left_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    right_fan_ready, right_fan_valid, right_fan_data = ac.queue[ac.u64](
        available & left_fan_ready & right_fan_ready,
        value,
        right_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    left_ready, left_valid, left_data = ac.queue[ac.u64](
        left_fan_valid,
        left_fan_data + 1,
        merged_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    right_ready, right_valid, right_data = ac.queue[ac.u64](
        right_fan_valid,
        right_fan_data + 2,
        merged_ready & ~left_valid,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    merged_ready, out_valid, out_data = ac.queue[ac.u64](
        left_valid | right_valid,
        left_data if left_valid else right_data,
        take,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    return BroadcastResult(ready=ready, valid=out_valid, data=out_data)
