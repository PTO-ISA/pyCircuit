# ruff: noqa: F823, F841, N802 -- Captured Module state.
"""A u2 selector does not prove a three-element Table access in range."""

import pycircuit as ac


@ac.module
def Top():
    regs: ac.table[3, ac.u8] = (1, 2, 3)
    index: ac.u2 = 0

    @ac.autorule
    def select() -> None:
        ac.log("info", "selected", regs[index])
        index += 1
