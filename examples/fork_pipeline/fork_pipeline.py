# ruff: noqa: N802, F821 -- hardware naming and current forward queue bindings.
"""Decoupled unsigned64 fanout with independent delivery tracking."""

import pycircuit as ac


@ac.struct
class ForkResult:
    ready: ac.u1
    left_valid: ac.u1
    left_data: ac.u64
    right_valid: ac.u1
    right_data: ac.u64


@ac.rule
def track_delivery(
    left_done,
    right_done,
    available,
    complete,
    left_accepted,
    right_accepted,
    ready,
    left_valid,
    left_data,
    right_valid,
    right_data,
) -> ForkResult:
    result = ForkResult(
        ready=ready,
        left_valid=left_valid,
        left_data=left_data,
        right_valid=right_valid,
        right_data=right_data,
    )
    if available:
        left_done = 0 if complete else left_done | left_accepted
        right_done = 0 if complete else right_done | right_accepted
    return result


@ac.module
def ForkPipeline(
    valid: ac.u1, data: ac.u64, left_take: ac.u1, right_take: ac.u1
) -> ForkResult:
    left_done: ac.u1 = 0
    right_done: ac.u1 = 0
    ready, available, head = ac.queue[ac.u64](
        valid,
        data,
        (left_done | (left_room & ~left_done))
        & (right_done | (right_room & ~right_done)),
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    left_room, left_valid, left_data = ac.queue[ac.u64](
        available & ~left_done,
        head,
        left_take,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    right_room, right_valid, right_data = ac.queue[ac.u64](
        available & ~right_done,
        head,
        right_take,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    left_accepted = available & ~left_done & left_room
    right_accepted = available & ~right_done & right_room
    complete = available & (left_done | left_accepted) & (right_done | right_accepted)
    return track_delivery(
        left_done,
        right_done,
        available,
        complete,
        left_accepted,
        right_accepted,
        ready,
        left_valid,
        left_data,
        right_valid,
        right_data,
    )
