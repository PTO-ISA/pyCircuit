"""Clean same-unit ancestor call; harness also reverses declaration order."""

# ruff: noqa: F841 -- allocations establish hardware ownership.
import pycircuit as ac


@ac.module
def Parent():
    value: ac.u8 = 7

    @ac.method
    def read() -> ac.u8:
        return value

    child = Child()


@ac.module
def Child():
    parent = ac.instance_ref(Parent, "..")

    @ac.rule
    def sample() -> None:
        value = parent.read()
        assert value == 7, "ancestor value"
        ac.log("info", "ancestor", value)

    sample()


@ac.system(samples=((0, 0), (1, 0), (1, 0), (0, 0), (1, 1), (0, 0), (1, 0)))
def Top():
    parent = Parent()
