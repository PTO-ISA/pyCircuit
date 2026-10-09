import pycircuit as ac

@ac.struct
class Result:
    value: ac.u1

@ac.rule
def update(state, first, second) -> Result:
    saved = state
    assert state, "entry"
    state = first
    assert state, "between"
    state = second
    assert state, "after"
    assert saved, "saved"
    return Result(value=state)

@ac.module
def Top(first: ac.u1, second: ac.u1) -> Result:
    state: ac.u1 = 0
    return update(state, first, second)
