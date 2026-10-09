"""Independent nominal provider for Table fields; no consumer source fallback."""
import pycircuit as ac


@ac.struct
class Cell:
    data: ac.u7
    stamp: ac.u2


@ac.struct
class Frame:
    lead: ac.u3
    cells: ac.table[3, Cell]
    trail: ac.u1


@ac.module
def Produce(data: ac.u7) -> Frame:  # noqa: N802
    return Frame(lead=5, cells=(Cell(data=data, stamp=1),
                                Cell(data=65, stamp=2),
                                Cell(data=11, stamp=3)), trail=1)
