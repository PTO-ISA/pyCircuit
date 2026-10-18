# ruff: noqa: B011, F823, F841 -- captured state updates and hardware assertions.
from enum import Enum
from typing import Annotated

import pycircuit as ac


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


@ac.module
def FiniteStateOwner():
    wide: ac.bits[73] = 4740813226943354765310
    logical: bool = True
    bounded: Annotated[int, range(1 << 5)] = 3
    tag: Tag = Tag.ZERO
    packet: Packet = Packet(
        left=Cell(data=4, tag=Tag.ZERO), right=Cell(data=8, tag=Tag.FIVE)
    )
    rows = ac.table[2, Cell](init=Cell(data=2, tag=Tag.ZERO))

    @ac.method(guard=lambda allow: allow)
    def advance(allow: ac.u1) -> None:
        saved = packet
        saved_rows = rows
        wide = wide + 3
        logical = not logical
        bounded = (bounded + 7) & 31
        tag = Tag.FIVE if tag == Tag.ZERO else Tag.ZERO
        packet.left.data = packet.left.data + 2
        if logical:
            packet.right = Cell(data=11, tag=Tag.ZERO)
            rows[1].data = rows[0].data + 2
        else:
            packet.right = Cell(data=9, tag=Tag.FIVE)
            rows[1].data = rows[0].data + 1
        rows[0] = packet.left
        assert packet.left.data == saved.left.data + 2
        expected_row = saved_rows[0].data + 2 if logical else saved_rows[0].data + 1
        assert rows[1].data == expected_row

    @ac.method
    def observe() -> None:
        ac.log(
            "info",
            "finite state",
            wide[:64],
            wide[64:73],
            logical,
            bounded,
            ac.enum_to_bits(tag),
            packet.left.data,
            packet.right.data,
            ac.enum_to_bits(packet.right.tag),
            rows[0].data,
            rows[1].data,
            ac.enum_to_bits(rows[1].tag),
        )
