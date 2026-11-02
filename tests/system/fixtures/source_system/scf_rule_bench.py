# ruff: noqa: F823, F841 -- captured module-owned state updates.
import pycircuit as ac
from checks.scf_rule_provider import ScfRuleOwner


@ac.system
def ExerciseScfRules():
    phase: ac.u3 = 0
    owner = ScfRuleOwner()

    @ac.autorule
    def drive() -> None:
        before, local, left, right = owner.advance(phase)
        ac.log(
            "info",
            "scf candidate",
            before,
            local.data,
            ac.enum_to_bits(local.tag),
            left,
            right,
        )

    @ac.autorule
    def inspect() -> None:
        owner.snapshot()

    @ac.autorule
    def tick() -> None:
        phase = phase + 1
