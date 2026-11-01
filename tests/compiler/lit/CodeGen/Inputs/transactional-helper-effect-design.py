# ruff: noqa: N802 -- Captured Module name.
"""An unused ordinary function still cannot hide a transaction effect."""

import pycircuit as ac


def impure(value: ac.u8) -> ac.u8:
    ac.log("info", "hidden", value)
    return value


@ac.module
def Top() -> None:
    pass
