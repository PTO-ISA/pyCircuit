# ruff: noqa: B007, F821, F823, F841 -- capture-only dead names and state proposals.
import pycircuit as ac
from checks.scf_nested_for_provider import NestedOwner


@ac.system
def ExerciseNestedMethod():
    phase: ac.u2 = 0
    owner = NestedOwner()

    @ac.rule
    def advance_owner() -> None:
        candidate = owner.advance(phase)
        ac.log("info", "nested method", candidate)

    @ac.rule
    def tick() -> None:
        phase = phase + 1
