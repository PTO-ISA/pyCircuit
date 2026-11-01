# ruff: noqa: F823, F841, N802 -- Captured Module state.
"""Static expansion does not make overlapping state writes admissible."""

import pycircuit as ac


@ac.module
def Overlap():
    saved: ac.u8 = 0

    @ac.autorule
    def update() -> None:
        for _iteration in range(2):
            saved += 1
