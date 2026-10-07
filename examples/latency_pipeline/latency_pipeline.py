# ruff: noqa: N802, F821 -- hardware naming and existing forward queue binding.
"""A two-slot input and four-slot delayed result preserve six-token capacity."""

import pycircuit as ac


@ac.struct
class LatencyResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u64


@ac.module
def LatencyPipeline(valid: ac.u1, data: ac.u64, take: ac.u1) -> LatencyResult:
    ready, available, value = ac.queue[ac.u64](
        valid,
        data,
        stage_ready,
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
    )
    incremented = value + 1
    stage_ready, out_valid, out_data = ac.queue[ac.u64](
        available,
        incremented,
        take,
        depth=4,
        latency=3,
        ready_policy="downstream_pop",
    )
    return LatencyResult(ready=ready, valid=out_valid, data=out_data)
