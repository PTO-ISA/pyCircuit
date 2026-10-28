# ruff: noqa: F841 -- captured formal proposals are hardware effects.
import pycircuit as ac


@ac.module
def FormalDemandProvider() -> ac.u8:
    @ac.rule(guard=lambda: False)
    def denied() -> ac.u8:
        return 99

    return denied
