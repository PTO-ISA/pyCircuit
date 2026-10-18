# ruff: noqa: N802 -- hardware root name.
"""A two-slot input and four-slot delayed result preserve six-token capacity."""

import pycircuit as ac


@ac.struct
class LatencyResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u64


@ac.module
def LatencyPipeline(valid: ac.u1, data: ac.u64, take: ac.u1) -> LatencyResult:
    ingress = ac.queue[ac.u64](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[ac.u64](
        depth=4, latency=3, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        value = ingress.pop()
        incremented = value + 1
        egress.push(incremented)

    transfer()
    return LatencyResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
