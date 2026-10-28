import pycircuit as ac
from checks.method_effects_provider import MethodCounter


@ac.system
def ExerciseMethodEffects():
    owner = MethodCounter()
    suppressed_owner = MethodCounter()

    @ac.rule
    def drive() -> None:
        first = owner.bump(1)
        second = owner.bump(2)
        owner.verify(10)
        owner.snapshot()
        ac.log("info", "saved candidates", first, second)
        ac.log("debug", "literal only", "fixed")

    @ac.rule
    def suppress() -> None:
        suppressed_owner.suppressed()
