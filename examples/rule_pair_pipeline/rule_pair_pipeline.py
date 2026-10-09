"""Independent buffered unsigned64-bit increment and double streams."""

import pycircuit as ac


@ac.struct
class PairResult:
    left_ready: ac.u1
    left_valid: ac.u1
    left_data: ac.u64
    right_ready: ac.u1
    right_valid: ac.u1
    right_data: ac.u64


@ac.module
def RulePairPipeline(  # noqa: N802
    left_valid: ac.u1,
    left_data: ac.u64,
    left_take: ac.u1,
    right_valid: ac.u1,
    right_data: ac.u64,
    right_take: ac.u1,
) -> PairResult:
    left_ready, left_available, left_value = ac.queue[ac.u64](
        left_valid,
        left_data,
        left_stage_ready,  # noqa: F821
        depth=1,
        ready_policy="downstream_pop",
    )
    incremented = left_value + 1
    left_stage_ready, left_out_valid, left_out_data = ac.queue[ac.u64](
        left_available, incremented, left_take, depth=1, ready_policy="downstream_pop"
    )
    right_ready, right_available, right_value = ac.queue[ac.u64](
        right_valid,
        right_data,
        right_stage_ready,  # noqa: F821
        depth=1,
        ready_policy="downstream_pop",
    )
    doubled = right_value * 2
    right_stage_ready, right_out_valid, right_out_data = ac.queue[ac.u64](
        right_available, doubled, right_take, depth=1, ready_policy="downstream_pop"
    )
    return PairResult(
        left_ready=left_ready,
        left_valid=left_out_valid,
        left_data=left_out_data,
        right_ready=right_ready,
        right_valid=right_out_valid,
        right_data=right_out_data,
    )
