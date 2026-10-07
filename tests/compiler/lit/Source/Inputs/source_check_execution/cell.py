# ruff: noqa: N802, F841 -- HDL module names and effectful unused child.
import pycircuit as ac
from execution.guard import Guard


@ac.struct
class Result:
    value: ac.u1


@ac.rule
def update(state, data, allow) -> Result:
    old = state
    assert allow, "cell"
    state = data
    return Result(value=old)


@ac.module
def Cell(data: ac.u1, allow: ac.u1) -> Result:
    state: ac.u1 = 0
    checked = Guard(allow)
    return update(state, data, allow)
