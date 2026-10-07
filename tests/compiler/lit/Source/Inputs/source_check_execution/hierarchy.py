# ruff: noqa: N802 -- HDL module names.
import pycircuit as ac
from execution.cell import Cell


@ac.struct
class Result:
    left: ac.u1
    right: ac.u1


@ac.module
def Top(data: ac.u1, allow_a: ac.u1, allow_b: ac.u1) -> Result:
    zzz = Cell(data, allow_a)
    aaa = Cell(data, allow_b)
    return Result(left=zzz.value, right=aaa.value)
