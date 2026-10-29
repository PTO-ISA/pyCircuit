import pycircuit as ac
from example_parent_state_interfaces.rules import add_one
from example_parent_state_interfaces.stage import Export, Stage


@ac.system
def Top():  # noqa: N802
    left: ac.u8 = 3
    right: ac.u8 = 10
    incremented: ac.u8 = 0
    first: ac.u8 = 20
    second: ac.u8 = 30
    incremented = add_one(left)
    first = Stage(incremented)
    second = Stage(right)
    alias = Export(first)

    @ac.autorule
    def observe() -> None:
        ac.log("info", "parent", left, right, incremented, first, second, alias)
