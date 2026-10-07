from typing import Annotated
from pycircuit import module


@module
def Mask(a: Annotated[int, range(1 << 8)], *, MASK: int = 255
         ) -> {"left": Annotated[int, range(1 << 8)],
               "right": Annotated[int, range(1 << 8)]}:
    return {"left": MASK & a, "right": a & MASK}


@module
def Computed(a: Annotated[int, range(1 << (4 + 4))],
             b: Annotated[int, range(1 << (4 + 4))]
             ) -> {"q": Annotated[int, range(1 << (4 + 4))],
                   "fifteen_left": Annotated[int, range(1 << (4 + 4))],
                   "fifteen_right": Annotated[int, range(1 << (4 + 4))]}:
    return {"q": a & b, "fifteen_left": 15 & a, "fifteen_right": a & 15}
