"""Atomic full-u64 conversion results with four independent output queues."""
# ruff: noqa: F821, N802 -- forward hardware queue-result wires.

import pycircuit as ac


@ac.struct
class FullResult:
    ready: ac.u1
    wrapped_valid: ac.u1
    wrapped: ac.u64
    saturated_valid: ac.u1
    saturated: ac.u64
    checked_value_valid: ac.u1
    checked_value: ac.u64
    checked_flag_valid: ac.u1
    checked_flag: ac.u1


@ac.module
def BoundedFullU64(
    valid: ac.u1,
    raw: ac.u64,
    take_wrapped: ac.u1,
    take_saturated: ac.u1,
    take_checked_value: ac.u1,
    take_checked_flag: ac.u1,
) -> FullResult:
    ready, source_valid, source_value = ac.queue[ac.u64](
        valid,
        raw,
        wrapped_ready & saturated_ready & checked_value_ready & checked_flag_ready,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    wrapped_ready, wrapped_valid, wrapped = ac.queue[ac.u64](
        (
            source_valid & wrapped_ready & saturated_ready
            & checked_value_ready & checked_flag_ready
        ),
        source_value,
        take_wrapped,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    saturated_ready, saturated_valid, saturated = ac.queue[ac.u64](
        (
            source_valid & wrapped_ready & saturated_ready
            & checked_value_ready & checked_flag_ready
        ),
        source_value,
        take_saturated,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    checked_value_ready, checked_value_valid, checked_value = ac.queue[ac.u64](
        (
            source_valid & wrapped_ready & saturated_ready
            & checked_value_ready & checked_flag_ready
        ),
        source_value,
        take_checked_value,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    checked_flag_ready, checked_flag_valid, checked_flag = ac.queue[ac.u1](
        (
            source_valid & wrapped_ready & saturated_ready
            & checked_value_ready & checked_flag_ready
        ),
        1,
        take_checked_flag,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    return FullResult(
        ready=ready,
        wrapped_valid=wrapped_valid,
        wrapped=wrapped,
        saturated_valid=saturated_valid,
        saturated=saturated,
        checked_value_valid=checked_value_valid,
        checked_value=checked_value,
        checked_flag_valid=checked_flag_valid,
        checked_flag=checked_flag,
    )
