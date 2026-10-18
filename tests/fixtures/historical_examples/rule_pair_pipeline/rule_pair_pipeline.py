"""Independent buffered unsigned64-bit increment and double streams."""

import pycircuit as ac


@ac.struct
class Channel:
    ready: ac.u1
    valid: ac.u1
    data: ac.u64


@ac.module
def RulePairPipeline(  # noqa: N802
    left_valid: ac.u1,
    left_data: ac.u64,
    left_take: ac.u1,
    right_valid: ac.u1,
    right_data: ac.u64,
    right_take: ac.u1,
) -> tuple[Channel, Channel]:
    left_ingress = ac.queue[ac.u64](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=left_valid,
        in_data=left_data,
    )
    left_egress = ac.queue[ac.u64](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=left_take
    )
    right_ingress = ac.queue[ac.u64](
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=right_valid,
        in_data=right_data,
    )
    right_egress = ac.queue[ac.u64](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=right_take
    )

    @ac.rule
    def transfer_left() -> None:
        left_value = left_ingress.pop()
        incremented = left_value + 1
        left_egress.push(incremented)

    @ac.rule
    def transfer_right() -> None:
        right_value = right_ingress.pop()
        doubled = right_value * 2
        right_egress.push(doubled)

    transfer_left()
    transfer_right()
    return (
        Channel(
            ready=left_ingress.ready,
            valid=left_egress.valid,
            data=left_egress.data,
        ),
        Channel(
            ready=right_ingress.ready,
            valid=right_egress.valid,
            data=right_egress.data,
        ),
    )
