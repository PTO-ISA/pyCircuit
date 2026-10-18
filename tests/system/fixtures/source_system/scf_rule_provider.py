# ruff: noqa: F823, F841 -- captured module-owned state updates.
from enum import Enum

import pycircuit as ac


@ac.encoding(width=3)
class Tag(Enum):
    ZERO = 0
    FIVE = 5


@ac.struct
class Cell:
    data: ac.u8
    tag: Tag


@ac.module
def ScfRuleOwner():
    total: ac.u8 = 5
    rows = ac.table[3, Cell](init=Cell(data=0, tag=Tag.ZERO))

    @ac.rule
    def pair(value: ac.u8) -> tuple[ac.u8, ac.u8]:
        return value + 1, value + 2

    @ac.rule
    def advance(limit: ac.u3) -> tuple[ac.u8, Cell, ac.u8, ac.u8]:
        before = total
        local = Cell(data=0, tag=Tag.ZERO)
        left: ac.u8 = 0
        right: ac.u8 = 0
        for index in range(3):
            local.data = local.data + rows[index].data
            rows[index].data = rows[index].data + 1
            left, right = pair(left + right)
        count: ac.u3 = 0
        while count < limit:
            local = Cell(
                data=local.data + 2,
                tag=Tag.FIVE if local.tag == Tag.ZERO else Tag.ZERO,
            )
            count = count + 1
        total = total + local.data
        return before, local, left, right

    @ac.rule
    def snapshot() -> None:
        ac.log("info", "scf state", total, rows[0].data, rows[1].data, rows[2].data)
