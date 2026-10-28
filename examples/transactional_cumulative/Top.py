# ruff: noqa: F821, F823, F842, N802 -- Compiler-owned hardware bindings.
import pycircuit as ac
from example_transactional_cumulative.design import Pipeline


@ac.system
def Top():
    sent: ac.u8 = 0
    received: ac.u8 = 0
    cycle: ac.u8 = 0

    @ac.rule(guard=lambda: sent < 5)
    def produce() -> ac.u8:
        value: ac.u8 = (
            3
            if (sent == 0) | (sent == 4)
            else 5 if sent == 1 else 2 if sent == 2 else 7
        )
        sent = sent + 1
        ac.log("info", "send", value)
        return value

    incoming = produce()
    totals = Pipeline(incoming)

    @ac.rule(guard=lambda: cycle >= 10)
    def consume(value: ac.u16) -> None:
        received = received + 1
        ac.log("info", "receive", value)

    consume(totals)

    @ac.autorule
    def tick() -> None:
        cycle = cycle + 1
        ac.log("info", "progress", sent, received)
