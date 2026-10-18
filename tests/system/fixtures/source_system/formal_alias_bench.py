# ruff: noqa: F823, F841 -- captured module state proposals.
import pycircuit as ac
from checks.formal_alias_provider import AliasOwner, FormalCalculator


@ac.system
def ExerciseFormalAliases():
    phase: ac.u3 = 0
    count: ac.u8 = 19
    forwarded: ac.u8 = 41
    owner = AliasOwner()
    calculator = FormalCalculator()

    @ac.rule
    def update_alias() -> None:
        owner.advance(phase != 1)

    @ac.rule
    def inspect_alias() -> None:
        owner.inspect()

    @ac.rule
    def update_formal() -> None:
        before = calculator.revise(count, phase != 1, phase == 2)
        ac.log("info", "formal imported", before, count)

    @ac.rule
    def exercise_forwarding() -> None:
        snapshot = forwarded
        simple_value = calculator.forward(snapshot)
        direct_value = calculator.direct_then_forward(snapshot)
        simple_owner = calculator.forward(forwarded)
        direct_owner = calculator.direct_then_forward(forwarded)
        ac.log(
            "info",
            "formal forwarded",
            snapshot,
            simple_value,
            direct_value,
            simple_owner,
            direct_owner,
            forwarded,
        )

    @ac.rule
    def tick() -> None:
        phase = phase + 1

    update_alias()
    inspect_alias()
    update_formal()
    exercise_forwarding()
    tick()
