# ruff: noqa: F841, N802 -- Captured Module state.
"""One overlapping transaction and unchanged cross-Rule resource conflict."""

import pycircuit as ac


@ac.module
def Overlap():
    word: ac.u8 = 0

    @ac.autorule
    def update() -> None:
        word[:5] = ac.bits[5](3)
        word[4:8] = ac.u4(2)


@ac.module
def CrossRule():
    word: ac.u8 = 0

    @ac.autorule
    def low() -> None:
        word[:4] = ac.u4(1)

    @ac.autorule
    def high() -> None:
        word[4:8] = ac.u4(2)
