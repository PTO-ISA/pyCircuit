# ruff: noqa: F823, F841 -- captured module-owned state updates.
import pycircuit as ac
from checks.rule_queue_provider import AtomicQueuePair, RuleQueue


@ac.system
def ExerciseRuleQueue():
    phase: ac.u8 = 0
    owner = RuleQueue()

    @ac.rule
    def drive() -> None:
        if phase < 2:
            owner.send(phase + 10)
        elif phase < 4:
            old = owner.replace(phase + 18)
            ac.log("info", "queue replaced", phase, old)
        else:
            old = owner.take()
            ac.log("info", "queue taken", phase, old)

    @ac.rule
    def tick() -> None:
        phase = phase + 1


@ac.system
def ExerciseAtomicQueuePaths():
    phase: ac.u8 = 0
    owner = AtomicQueuePair()

    @ac.rule
    def drive() -> None:
        if phase == 0:
            owner.seed_left()
        elif phase == 1:
            old = owner.exchange()
            ac.log("info", "queue blocked resources", phase, old)
        elif phase == 2:
            old = owner.denied()
            ac.log("info", "queue denied call", phase, old)
        elif phase == 3:
            owner.seed_right()
        elif phase == 4:
            old = owner.exchange()
            ac.log("info", "queue atomic exchange", phase, old)
        elif phase == 5:
            old = owner.pick(1)
            ac.log("info", "queue selected path", phase, old)
        else:
            old = owner.pick(0)
            ac.log("info", "queue empty path", phase, old)

    @ac.rule
    def tick() -> None:
        phase = phase + 1
