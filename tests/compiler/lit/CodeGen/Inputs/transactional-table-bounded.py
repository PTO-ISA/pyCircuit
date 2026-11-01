# ruff: noqa: F823, F841, N802 -- Captured Module state.
"""A wider selector clamped by ordinary local control stays within the Table."""

import pycircuit as ac


@ac.module
def IndexBank():
    values: ac.table[3, ac.u8] = (11, 22, 33)
    phase: ac.u3 = 0

    @ac.autorule
    def select() -> None:
        index = phase
        if index >= 3:
            index = 2
        ac.log("info", "clamped", phase, values[index])
        phase += 1


@ac.system
def Top():
    IndexBank()
