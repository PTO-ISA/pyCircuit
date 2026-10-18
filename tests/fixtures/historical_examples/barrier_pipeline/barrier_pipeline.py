# ruff: noqa: N802 -- hardware module name.
"""Atomic positional movement of two independently buffered typed streams."""

import pycircuit as ac


@ac.struct
class LeftToken:
    value: ac.u16


@ac.struct
class RightToken:
    value: ac.u32


@ac.struct
class LeftChannel:
    ready: ac.u1
    valid: ac.u1
    data: LeftToken


@ac.struct
class RightChannel:
    ready: ac.u1
    valid: ac.u1
    data: RightToken


@ac.module
def BarrierPipeline(
    left_valid: ac.u1,
    left_data: LeftToken,
    left_take: ac.u1,
    right_valid: ac.u1,
    right_data: RightToken,
    right_take: ac.u1,
) -> tuple[LeftChannel, RightChannel]:
    left_ingress = ac.queue[LeftToken](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=left_valid,
        in_data=left_data,
    )
    right_ingress = ac.queue[RightToken](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=right_valid,
        in_data=right_data,
    )
    left_egress = ac.queue[LeftToken](
        depth=2, latency=1, ready_policy="downstream_pop", out_ready=left_take
    )
    right_egress = ac.queue[RightToken](
        depth=2, latency=1, ready_policy="downstream_pop", out_ready=right_take
    )

    @ac.rule
    def transfer() -> None:
        left = left_ingress.pop()
        right = right_ingress.pop()
        left_egress.push(left)
        right_egress.push(right)

    transfer()
    return (
        LeftChannel(
            ready=left_ingress.ready,
            valid=left_egress.valid,
            data=left_egress.data,
        ),
        RightChannel(
            ready=right_ingress.ready,
            valid=right_egress.valid,
            data=right_egress.data,
        ),
    )
