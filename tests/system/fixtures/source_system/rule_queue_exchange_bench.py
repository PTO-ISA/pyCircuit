# ruff: noqa: F823, F841 -- captured module-owned state proposals.
import pycircuit as ac
from checks.rule_queue_exchange_provider import ConditionalQueueExchange


@ac.system
def ExerciseConditionalQueueExchange():
    cycle: ac.u8 = 0
    selected: ac.u1 = 0
    owner = ConditionalQueueExchange()

    @ac.rule
    def run() -> None:
        if cycle == 0:
            owner.seed()
        else:
            owner.step(cycle, selected)

    @ac.rule
    def tick() -> None:
        cycle = cycle + 1
        selected = ~selected

    run()
    tick()
