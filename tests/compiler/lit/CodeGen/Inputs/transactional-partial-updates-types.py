"""Published defaults remain constructor authority across source units."""

from enum import Enum

import pycircuit as ac

ACTIVE = True


@ac.encoding(width=2)
class Mode(Enum):
    IDLE = 0
    READ = 1
    WRITE = 2


@ac.struct
class Inner:
    tag: ac.u8 = 7
    flags: ac.u8 = -1
    active: bool = ACTIVE


@ac.struct
class Packet:
    raw: Inner
    preset: Inner = Inner(tag=3)
    mode: Mode = Mode.READ
