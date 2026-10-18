"""Atomic unsigned64 broadcast, two transforms and left-priority merge."""

import pycircuit as ac


@ac.struct
class BroadcastResult:
    ready: ac.u1
    valid: ac.u1
    data: ac.u64


@ac.module
def BroadcastPipeline(valid: ac.u1, data: ac.u64, take: ac.u1) -> BroadcastResult:  # noqa: N802
    ingress = ac.queue[ac.u64](depth=2, latency=1, in_valid=valid, in_data=data)
    left_fan = ac.queue[ac.u64](depth=1, latency=1)
    right_fan = ac.queue[ac.u64](depth=1, latency=1)
    left = ac.queue[ac.u64](depth=1, latency=1)
    right = ac.queue[ac.u64](depth=1, latency=1)
    egress = ac.queue[ac.u64](depth=2, latency=1, out_ready=take)

    @ac.rule
    def broadcast() -> None:
        value = ingress.pop()
        left_fan.push(value)
        right_fan.push(value)

    @ac.rule
    def transform_left() -> None:
        value = left_fan.pop()
        left.push(value + 1)

    @ac.rule
    def transform_right() -> None:
        value = right_fan.pop()
        right.push(value + 2)

    @ac.rule
    def merge() -> None:
        # Availability selects the authored left-priority arbitration policy.
        # The compiler guards the selected pop and output push atomically.
        if left.valid:
            value = left.pop()
        else:
            value = right.pop()
        egress.push(value)

    broadcast()
    transform_left()
    transform_right()
    merge()
    return BroadcastResult(ready=ingress.ready, valid=egress.valid, data=egress.data)
