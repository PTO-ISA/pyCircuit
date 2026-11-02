# ruff: noqa: F823, F841 -- captured module-owned state updates.
import pycircuit as ac
from checks.finite_state_provider import FiniteStateOwner


@ac.system
def ExerciseFiniteState():
    phase: ac.u3 = 0
    owner = FiniteStateOwner()
    isolated = FiniteStateOwner()

    @ac.autorule
    def drive() -> None:
        owner.advance(phase != 1)

    @ac.autorule
    def inspect() -> None:
        owner.observe()

    @ac.autorule
    def inspect_isolated() -> None:
        isolated.observe()

    @ac.autorule
    def tick() -> None:
        phase = phase + 1
