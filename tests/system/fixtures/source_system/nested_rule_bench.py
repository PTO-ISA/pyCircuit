# ruff: noqa: B011, F823, F841 -- captured state updates and hardware assertions.
import pycircuit as ac
from checks.nested_rule_provider import RuleRelay


@ac.system
def ExerciseNestedRules():
    phase: ac.u3 = 0
    left = RuleRelay()
    right = RuleRelay()

    @ac.rule
    def nested(delta: ac.u8) -> tuple[ac.u8, ac.u8]:
        before, after = left.add(delta)
        left.require(phase != 1)
        return before, after

    @ac.rule
    def drive() -> None:
        before, middle = nested(2)
        second_before, after = left.add(3)
        assert middle == before + 2
        assert second_before == middle
        ac.log("info", "nested accepted", after)

    @ac.rule
    def advance_independent() -> None:
        right.add(1)

    @ac.rule
    def inspect() -> None:
        ac.log("info", "nested owners", left.read(), right.read(), phase)

    @ac.rule
    def tick() -> None:
        phase = phase + 1

    drive()
    advance_independent()
    inspect()
    tick()
