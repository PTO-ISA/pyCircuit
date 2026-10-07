"""Four depth-two queues with three wrapping unsigned 64-bit increments."""

import pycircuit as ac


@ac.struct
class QueueResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u64


@ac.module
def RecursivePipeline(  # noqa: N802
    valid: ac.u1, data: ac.u64, take: ac.u1
) -> QueueResult:
    ready, source_valid, source_data = ac.queue[ac.u64](
        valid,
        data,
        stage1_ready,  # noqa: F821
        depth=2,
        ready_policy="downstream_pop",
        latency=1,
    )
    incremented1 = source_data + 1
    stage1_ready, stage1_valid, stage1_data = ac.queue[ac.u64](
        source_valid,
        incremented1,
        stage2_ready,  # noqa: F821
        depth=2,
        ready_policy="downstream_pop",
        latency=1,
    )
    incremented2 = stage1_data + 1
    stage2_ready, stage2_valid, stage2_data = ac.queue[ac.u64](
        stage1_valid,
        incremented2,
        stage3_ready,  # noqa: F821
        depth=2,
        ready_policy="downstream_pop",
        latency=1,
    )
    incremented3 = stage2_data + 1
    stage3_ready, out_valid, out_data = ac.queue[ac.u64](
        stage2_valid,
        incremented3,
        take,
        depth=2,
        ready_policy="downstream_pop",
        latency=1,
    )
    return QueueResult(ready=ready, valid=out_valid, data=out_data)
