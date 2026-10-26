"""A declared provider interface; harness also reverses consumer declaration order."""

# ruff: noqa: F841 -- allocations establish hardware ownership.
import pycircuit as ac


@ac.module
def Provider():
    value: ac.u8 = 7

    @ac.method
    def read() -> ac.u8:
        return value

@ac.module
def Parent():
    provider = Provider()
    child = Child(provider)


@ac.module
def Child(parent: Provider):

    @ac.rule
    def sample() -> None:
        value = parent.read()
        assert value == 7, "ancestor value"
        ac.log("info", "ancestor", value)



@ac.system(samples=((0, 0), (1, 0), (1, 0), (0, 0), (1, 1), (0, 0), (1, 0)))
def Top():
    parent = Parent()
