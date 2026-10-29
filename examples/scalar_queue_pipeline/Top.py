# ruff: noqa: F821, F823, F842 -- Module bindings and state are compiler-owned.
import pycircuit as ac
from example_scalar_queue_pipeline.Accumulator import Accumulator
from example_scalar_queue_pipeline.BiasStage import BiasStage
from example_scalar_queue_pipeline.SumStage import SumStage


@ac.system
def Top():  # noqa: N802
    sent: ac.bits[8] = 0
    received: ac.bits[8] = 0
    cycle: ac.bits[8] = 0
    accumulator: Accumulator

    @ac.rule(guard=lambda: sent < 4)
    def produce() -> ac.bits[8]:
        value: ac.bits[8] = (
            3 if sent == 0 else 5 if sent == 1 else 2 if sent == 2 else 7
        )
        sent = sent + 1
        ac.log("info", "send", value)
        return value

    incoming = produce()
    biased = ac.queue[ac.bits[8]](depth=3)
    biased = BiasStage(incoming)
    totals = SumStage(biased, accumulator)

    # Delaying consumption fills the final Queue and blocks the whole SumStage
    # transaction, including its call into the existing Accumulator instance.
    @ac.rule(guard=lambda: cycle >= 10)
    def consume(value: ac.bits[8]) -> None:
        expected: ac.bits[8] = (
            4 if received == 0 else 10 if received == 1 else 13 if received == 2 else 21
        )
        assert received < 4
        assert value == expected
        received = received + 1
        ac.log("info", "receive", value)

    consume(totals)

    @ac.autorule
    def tick() -> None:
        cycle = cycle + 1
        ac.log("info", "progress", sent, received)
