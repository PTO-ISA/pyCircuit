"""Two depth-two queues surrounding a wrapping unsigned64-bit increment."""

import pycircuit as ac


@ac.struct
class QueueResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u64


@ac.module
def QueuePipeline(valid: ac.u1, data: ac.u64, take: ac.u1) -> QueueResult:  # noqa: N802
    ingress = ac.queue[ac.u64](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[ac.u64](
        depth=2, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        value = ingress.pop()
        incremented = value + 1
        egress.push(incremented)

    transfer()
    return QueueResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
