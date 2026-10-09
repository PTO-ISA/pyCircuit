# ruff: noqa: N802, F821 -- HDL names and named-output annotations.
from pycircuit import module, rule


@module
def Top(path: bool, condition: bool) -> {"value": bool}:
    @rule
    def inspect():
        if path:
            assert condition, "matrix"
        else:
            pass
    inspect()
    return {"value": condition}
