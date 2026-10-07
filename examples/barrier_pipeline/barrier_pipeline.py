# ruff: noqa: N802, F821 -- hardware naming and current forward queue bindings.
"""Atomic positional movement of two independently buffered typed streams."""

import pycircuit as ac


@ac.struct
class LeftToken:
    value: ac.u16


@ac.struct
class RightToken:
    value: ac.u32


@ac.struct
class BarrierResult:
    left_ready: ac.u1
    left_valid: ac.u1
    left_data: LeftToken
    right_ready: ac.u1
    right_valid: ac.u1
    right_data: RightToken


@ac.module
def BarrierPipeline(
    left_valid: ac.u1,
    left_data: LeftToken,
    left_take: ac.u1,
    right_valid: ac.u1,
    right_data: RightToken,
    right_take: ac.u1,
) -> BarrierResult:
    left_ready, left_available, left_head = ac.queue[LeftToken](
        left_valid,
        left_data,
        right_available & left_room & right_room,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    right_ready, right_available, right_head = ac.queue[RightToken](
        right_valid,
        right_data,
        left_available & left_room & right_room,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    left_room, left_out_valid, left_out = ac.queue[LeftToken](
        left_available & right_available & right_room,
        left_head,
        left_take,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    right_room, right_out_valid, right_out = ac.queue[RightToken](
        left_available & right_available & left_room,
        right_head,
        right_take,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    return BarrierResult(
        left_ready=left_ready,
        left_valid=left_out_valid,
        left_data=left_out,
        right_ready=right_ready,
        right_valid=right_out_valid,
        right_data=right_out,
    )
