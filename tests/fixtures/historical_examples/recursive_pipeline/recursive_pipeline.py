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
    source = ac.queue[ac.u64](depth=2, latency=1, in_valid=valid, in_data=data)
    stage1 = ac.queue[ac.u64](depth=2, latency=1)
    stage2 = ac.queue[ac.u64](depth=2, latency=1)
    stage3 = ac.queue[ac.u64](depth=2, latency=1, out_ready=take)

    @ac.rule
    def first() -> None:
        stage1.push(source.pop() + 1)

    @ac.rule
    def second() -> None:
        stage2.push(stage1.pop() + 1)

    @ac.rule
    def third() -> None:
        stage3.push(stage2.pop() + 1)

    first()
    second()
    third()
    return QueueResult(ready=source.ready, valid=stage3.valid, data=stage3.data)
