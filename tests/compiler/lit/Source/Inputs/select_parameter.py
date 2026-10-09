from typing import Annotated
from pycircuit import module


@module
def Parameter(a: Annotated[int, range(1 << WIDTH)], flag: bool,
              *, WIDTH: int = 8, VALUE: int = 255
              ) -> {"right": Annotated[int, range(1 << WIDTH)],
                    "left": Annotated[int, range(1 << WIDTH)]}:
    return {"right": a if flag else VALUE, "left": VALUE if flag else a}
