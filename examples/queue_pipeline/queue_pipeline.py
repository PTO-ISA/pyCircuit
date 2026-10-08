"""Two depth-two queues surrounding a wrapping unsigned64-bit increment."""

import pycircuit as ac


@ac.struct
class QueueResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u64


@ac.module
def QueuePipeline(valid: ac.u1, data: ac.u64, take: ac.u1) -> QueueResult:  # noqa: N802
    ready, available, value = ac.queue[ac.u64](
        valid,
        data,
        stage_ready,  # noqa: F821
        depth=2,
        ready_policy="downstream_pop",
    )
    incremented = value + 1
    stage_ready, out_valid, out_data = ac.queue[ac.u64](
        available, incremented, take, depth=2, ready_policy="downstream_pop"
    )
    return QueueResult(ready=ready, valid=out_valid, data=out_data)
