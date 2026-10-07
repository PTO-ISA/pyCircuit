# ruff: noqa: N802 -- HDL module names.
import pycircuit as ac


@ac.struct
class Flag:
    value: ac.u1


@ac.rule
def inspect(condition) -> Flag:
    # The assertion still accepts exactly the known-one input. Zero now makes
    # the quotient X; zero/X/Z must all fail without committing any tree state.
    # The existing independent runtime acceptance/error oracles stay unchanged.
    assert condition // condition == 1, "grandchild"
    return Flag(value=condition)


@ac.module
def Guard(condition: ac.u1) -> Flag:
    return inspect(condition)
