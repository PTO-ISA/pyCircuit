"""One shared packet interface, including its nominal header encoding."""

from enum import Enum

import pycircuit as ac


@ac.encoding(width=2)
class Mode(Enum):
    IDLE = 0
    RAW = 1
    DONE = 2


@ac.struct
class Header:
    opcode: ac.u6
    mode: Mode


@ac.struct
class Packet:
    header: Header
    data: ac.bits[80]
