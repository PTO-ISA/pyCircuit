"""Atomic full-u64 conversion results with four independent output queues."""

# ruff: noqa: N802 -- hardware root name.

import pycircuit as ac


@ac.struct
class U64Channel:
    valid: ac.u1
    data: ac.u64


@ac.struct
class U1Channel:
    valid: ac.u1
    data: ac.u1


@ac.module
def BoundedFullU64(
    valid: ac.u1,
    raw: ac.u64,
    take_wrapped: ac.u1,
    take_saturated: ac.u1,
    take_checked_value: ac.u1,
    take_checked_flag: ac.u1,
) -> tuple[ac.u1, U64Channel, U64Channel, U64Channel, U1Channel]:
    ingress = ac.queue[ac.u64](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=raw,
    )
    wrapped_queue = ac.queue[ac.u64](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take_wrapped
    )
    saturated_queue = ac.queue[ac.u64](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take_saturated
    )
    checked_value_queue = ac.queue[ac.u64](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take_checked_value
    )
    checked_flag_queue = ac.queue[ac.u1](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take_checked_flag
    )

    @ac.rule
    def transfer() -> None:
        source_value = ingress.pop()
        wrapped_queue.push(source_value)
        saturated_queue.push(source_value)
        checked_value_queue.push(source_value)
        checked_flag_queue.push(1)

    transfer()
    return (
        ingress.ready,
        U64Channel(valid=wrapped_queue.valid, data=wrapped_queue.data),
        U64Channel(valid=saturated_queue.valid, data=saturated_queue.data),
        U64Channel(valid=checked_value_queue.valid, data=checked_value_queue.data),
        U1Channel(valid=checked_flag_queue.valid, data=checked_flag_queue.data),
    )
