# Captured hardware source: Rules update Module-owned state.
# ruff: noqa: N802, F823, F841
import pycircuit as ac


@ac.module
def Sync(enable: ac.u1) -> ac.u9:
    count: ac.u9 = ac.state(init=5, reset_value=0)

    @ac.rule
    def advance():
        if enable:
            count = count + 1

    return count


@ac.module
def Async(enable: ac.u1, rst_n: ac.u1) -> ac.u9:
    count: ac.u9 = ac.state(
        init=5,
        reset=rst_n,
        reset_kind="async",
        reset_polarity="low",
        reset_value=0,
        edge="negedge",
    )

    @ac.rule
    def advance():
        if enable:
            count = count + 1

    return count


@ac.module
def Uninitialized(enable: ac.u1) -> ac.u9:
    count: ac.u9 = ac.state(reset_kind="none")

    @ac.rule
    def load():
        if enable:
            count = 9

    return count
