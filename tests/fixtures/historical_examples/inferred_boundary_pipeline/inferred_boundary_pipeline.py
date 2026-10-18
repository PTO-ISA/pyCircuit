"""An eight-bit increment between the original two token stages."""

import pycircuit as ac
from pycircuit import module, rule


@ac.struct
class IncrementResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u8


@module
def InferredBoundaryPipeline(  # noqa: N802
    valid: ac.u1, data: ac.u8, take: ac.u1
) -> IncrementResult:
    ingress = ac.queue[ac.u8](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[ac.u8](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        value = ingress.pop()
        incremented = value + 1
        egress.push(incremented)

    transfer()
    return IncrementResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
