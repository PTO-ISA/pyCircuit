# ruff: noqa: B011 -- four-state assertion probe.
import pycircuit as ac
from checks.method_conditional_provider import ConditionalCounter


@ac.system
def ConditionalProbe():
    selector: ac.u1 = 0
    allow: ac.u1 = 1
    prior: ac.u1 = 1
    later: ac.u1 = 1
    early_check: ac.u1 = 1
    branch_check: ac.u1 = 1
    owner = ConditionalCounter()
    sibling = ConditionalCounter()

    @ac.rule
    def drive() -> None:
        owner.stop(prior)
        first = owner.add(1)
        assert early_check, "before conditional selector"
        if selector:
            selected = owner.guarded_add(allow, 2)
            assert branch_check, "selected conditional assertion"
            ac.log("info", "conditional true", selected)
        else:
            selected = owner.add(4)
            ac.log("info", "conditional false", selected)
        owner.stop(later)
        after = owner.add(3)
        sibling.add(1)
        owner.snapshot()
        ac.log("info", "probe snapshots", first, selected, after)
