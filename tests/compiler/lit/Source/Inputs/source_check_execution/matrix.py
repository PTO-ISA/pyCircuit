# ruff: noqa: N802, F821 -- HDL names and named-output annotations.
from pycircuit import module, rule


def implication(path: bool, condition: bool) -> bool:
    # This fixture checks combinational P -> C validity for all four states.
    # Procedural Rule branch selector knownness has separate execution tests.
    return (not path) or condition


@module
def Top(path: bool, condition: bool) -> {"value": bool}:
    @rule
    def inspect():
        assert implication(path, condition), "matrix"
    inspect()
    return {"value": condition}
