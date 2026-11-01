# ruff: noqa: F823, F841, N802 -- Captured System state.
"""Several specializations share one independently compiled loop template."""

import pycircuit as ac
from static_for_case.provider import Kernel, LoopState, Pipeline


@ac.rule
def report_sum(value: ac.u8) -> None:
    result = value
    for index in range(3):
        result += ac.u8(index)
    ac.log("info", "template", result)


@ac.system
def Top():
    Kernel(COUNT=0)
    Kernel(COUNT=2)
    Kernel(COUNT=4)
    LoopState()
    sent: ac.u8 = 0
    report_sum(sent)

    @ac.rule(guard=lambda: sent < 4)
    def produce() -> ac.u8:
        value = sent + 10
        sent += 1
        ac.log("info", "send", value)
        return value

    incoming = produce()
    outgoing = Pipeline(incoming, STAGES=3)

    @ac.rule
    def consume(value: ac.u8) -> None:
        ac.log("info", "receive", value)

    consume(outgoing)

    short_sent: ac.u8 = 0

    @ac.rule(guard=lambda: short_sent < 2)
    def produce_short() -> ac.u8:
        value = short_sent + 30
        short_sent += 1
        return value

    short_incoming = produce_short()
    short_outgoing = Pipeline(short_incoming, STAGES=1)

    @ac.rule
    def consume_short(value: ac.u8) -> None:
        ac.log("info", "receive_short", value)

    consume_short(short_outgoing)


@ac.system(samples=((0, 0), (1, 0), (0, 0), (1, 0), (0, 1), (1, 1), (0, 0), (1, 0)))
def ResetTop():
    Kernel(COUNT=2)


@ac.system
def FailureTop():
    phase: ac.u8 = 0
    lanes: ac.u8 = 0

    @ac.autorule
    def update() -> None:
        for bit in range(2):
            lanes[bit] = ac.u1(1)
            ac.log("info", "attempt", phase, ac.u8(bit), lanes)
            assert phase < 2
        phase += 1
