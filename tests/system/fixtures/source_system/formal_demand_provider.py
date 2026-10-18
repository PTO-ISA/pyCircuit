# ruff: noqa: F841 -- captured formal proposals are hardware effects.
import pycircuit as ac


@ac.module
def FormalDemandProvider():
    @ac.rule(guard=lambda: False)
    def denied() -> ac.u8:
        return 99

    unavailable = denied()

    @ac.rule
    def overwrite(value: ac.u8) -> None:
        value = unavailable
        value = 37
