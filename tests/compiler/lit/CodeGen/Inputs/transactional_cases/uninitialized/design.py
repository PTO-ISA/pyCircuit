# ruff: noqa: F821, F841, N802 -- Compiler-owned typed state and output bindings.
from enum import Enum

import pycircuit as ac


@ac.encoding(width=2)
class Mode(Enum):
    IDLE = 0
    ACTIVE = 2


@ac.struct
class Header:
    mode: Mode
    data: ac.bits[80]


@ac.module
def Top() -> tuple[ac.u8, Mode, Header]:
    saved: ac.u8
    mode: Mode
    header: Header
    written: ac.u1 = 0

    @ac.rule(guard=lambda: written == 0)
    def initialize() -> ac.u8:
        mode = Mode.ACTIVE
        header = Header(mode=Mode.ACTIVE, data=ac.bits[80](37))
        written = 1
        return 37

    saved = initialize()
    return saved, mode, header
