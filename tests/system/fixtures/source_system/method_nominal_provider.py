# ruff: noqa: B011, F823 -- hardware owner writes and source assertions.
from enum import Enum

import pycircuit as ac
from pycircuit import method, module


@ac.encoding(width=3)
class Tag(Enum):
    ZERO = 0
    FIVE = 5


@ac.struct
class Cell:
    data: ac.u8
    tag: Tag


@ac.struct
class Packet:
    left: Cell
    right: Cell


@ac.struct
class Result:
    packet: Packet
    saved: Packet
    direct: Tag


@module
def NominalCounter():
    count: ac.u8 = 5

    @method
    def advance(flag: ac.u1, delta: ac.u8) -> Result:
        before = count
        count = count + delta
        local = Packet(
            left=Cell(data=count, tag=Tag.ZERO), right=Cell(data=before, tag=Tag.ZERO)
        )
        saved = local
        local.left.data = count + 1
        if flag:
            local.left = Cell(data=count + 2, tag=Tag.FIVE)
        else:
            local.right = Cell(data=count + 10, tag=Tag.FIVE)
        direct = Tag.ZERO
        if flag:
            direct = Tag.FIVE
        assert count < 16, "nominal candidate must remain below limit"
        return Result(packet=local, saved=saved, direct=direct)

    @method(guard=lambda allow: allow)
    def guarded(allow: ac.u1) -> Packet:
        return Packet(
            left=Cell(data=count, tag=Tag.ZERO), right=Cell(data=count, tag=Tag.FIVE)
        )

    @method
    def read() -> ac.u8:
        return count
