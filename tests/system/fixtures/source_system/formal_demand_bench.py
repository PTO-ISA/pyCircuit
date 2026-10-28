# ruff: noqa: F823, F841 -- captured state and formal proposals.
import pycircuit as ac
from checks.formal_demand_provider import FormalDemandProvider


@ac.system
def ExerciseFormalDemands():
    owner: ac.u8 = 11
    blocked: ac.u8 = 0
    passed: ac.u8 = 0
    provider = FormalDemandProvider()

    @ac.rule
    def owned(value: ac.u8 = owner) -> None:
        value = provider
        value = 37
        blocked = blocked + 1
        ac.log("info", "formal owned attempt", owner, blocked)

    @ac.rule
    def by_value(value: ac.u8 = 13) -> None:
        local: ac.u8 = 13
        value = provider
        value = 37
        passed = passed + 1
        ac.log("info", "formal value attempt", local, passed)

    @ac.rule
    def inspect() -> None:
        ac.log("info", "formal demand snapshot", owner, blocked, passed)
