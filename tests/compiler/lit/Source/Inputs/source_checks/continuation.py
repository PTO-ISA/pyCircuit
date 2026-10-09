import pycircuit as ac

@ac.struct
class Result:
    value: ac.u1

@ac.rule
def inspect(gate, inner, selector, ok) -> Result:
    if gate:
        assert ok, "if-yes"
        if inner:
            assert True, "nested"
    else:
        pass
    assert True, "if-join"
    match selector:
        case 0:
            assert ok, "match-first"
        case _:
            pass
    assert True, "match-join"
    return Result(value=gate)

@ac.module
def Top(gate: ac.u1, inner: ac.u1, selector: ac.u1, ok: ac.u1) -> Result:
    return inspect(gate, inner, selector, ok)
