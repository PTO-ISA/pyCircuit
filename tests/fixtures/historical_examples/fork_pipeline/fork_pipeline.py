# ruff: noqa: N802, F823, F841 -- module-owned state updated inside Rules.
"""Decoupled unsigned64 fanout with independent delivery tracking."""

import pycircuit as ac


@ac.struct
class Channel:
    valid: ac.u1
    data: ac.u64


@ac.module
def ForkPipeline(
    valid: ac.u1, data: ac.u64, left_take: ac.u1, right_take: ac.u1
) -> tuple[ac.u1, Channel, Channel]:
    left_done: ac.u1 = 0
    right_done: ac.u1 = 0
    ingress = ac.queue[ac.u64](depth=1, latency=1, in_valid=valid, in_data=data)
    left = ac.queue[ac.u64](depth=2, latency=1, out_ready=left_take)
    right = ac.queue[ac.u64](depth=2, latency=1, out_ready=right_take)

    @ac.rule(guard=lambda: ~left_done)
    def send_left() -> None:
        value = ingress.peek()
        left.push(value)
        left_done = 1

    @ac.rule(guard=lambda: ~right_done)
    def send_right() -> None:
        value = ingress.peek()
        right.push(value)
        right_done = 1

    @ac.rule(after=(send_left, send_right), guard=lambda: left_done & right_done)
    def retire() -> None:
        ingress.pop()
        left_done = 0
        right_done = 0

    send_left()
    send_right()
    retire()
    return (
        ingress.ready,
        Channel(valid=left.valid, data=left.data),
        Channel(valid=right.valid, data=right.data),
    )
