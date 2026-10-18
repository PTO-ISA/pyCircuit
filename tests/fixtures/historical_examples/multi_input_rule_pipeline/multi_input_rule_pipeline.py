"""Atomic old-head addition of two buffered unsigned64-bit streams."""

import pycircuit as ac


@ac.struct
class Channel:
    valid: ac.u1
    data: ac.u64


@ac.module
def MultiInputRulePipeline(  # noqa: N802
    left_valid: ac.u1,
    left_data: ac.u64,
    right_valid: ac.u1,
    right_data: ac.u64,
    take: ac.u1,
) -> tuple[ac.u1, ac.u1, Channel]:
    left_ingress = ac.queue[ac.u64](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=left_valid,
        in_data=left_data,
    )
    right_ingress = ac.queue[ac.u64](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=right_valid,
        in_data=right_data,
    )
    egress = ac.queue[ac.u64](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def transfer() -> None:
        left_value = left_ingress.pop()
        right_value = right_ingress.pop()
        summed = left_value + right_value
        egress.push(summed)

    transfer()
    return (
        left_ingress.ready,
        right_ingress.ready,
        Channel(valid=egress.valid, data=egress.data),
    )
