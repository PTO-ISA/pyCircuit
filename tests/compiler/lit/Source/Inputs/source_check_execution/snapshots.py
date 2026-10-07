# ruff: noqa: N802 -- HDL module names.
import pycircuit as ac


@ac.struct
class Result:
    value: ac.u8


@ac.rule
def update(state, first, second, allow) -> Result:
    old = state
    state = first
    assert allow, "allow"
    assert state != 0, "between"
    state = second
    assert state != 0, "after"
    return Result(value=old)


@ac.module
def Top(first: ac.u8, second: ac.u8, allow: ac.u1) -> Result:
    state: ac.u8 = 3
    return update(state, first, second, allow)
