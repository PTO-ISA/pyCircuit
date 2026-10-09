"""Atomic old-head addition of two buffered unsigned64-bit streams."""

import pycircuit as ac


@ac.struct
class JoinResult:
    left_ready: ac.u1
    right_ready: ac.u1
    valid: ac.u1
    data: ac.u64


@ac.module
def MultiInputRulePipeline(  # noqa: N802
    left_valid: ac.u1,
    left_data: ac.u64,
    right_valid: ac.u1,
    right_data: ac.u64,
    take: ac.u1,
) -> JoinResult:
    left_ready, left_available, left_value = ac.queue[ac.u64](
        left_valid,
        left_data,
        right_available & stage_ready,  # noqa: F821
        depth=2,
        ready_policy="downstream_pop",
    )
    right_ready, right_available, right_value = ac.queue[ac.u64](
        right_valid,
        right_data,
        left_available & stage_ready,  # noqa: F821
        depth=2,
        ready_policy="downstream_pop",
    )
    summed = left_value + right_value
    stage_ready, out_valid, out_data = ac.queue[ac.u64](
        left_available & right_available,
        summed,
        take,
        depth=1,
        ready_policy="downstream_pop",
    )
    return JoinResult(
        left_ready=left_ready, right_ready=right_ready, valid=out_valid, data=out_data
    )
