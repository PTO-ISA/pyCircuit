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
    def owned() -> None:
        provider.overwrite(owner)
        blocked = blocked + 1
        ac.log("info", "formal owned attempt", owner, blocked)

    @ac.rule
    def by_value() -> None:
        local: ac.u8 = 13
        provider.overwrite(local)
        passed = passed + 1
        ac.log("info", "formal value attempt", local, passed)

    @ac.rule
    def inspect() -> None:
        ac.log("info", "formal demand snapshot", owner, blocked, passed)

    owned()
    by_value()
    inspect()
