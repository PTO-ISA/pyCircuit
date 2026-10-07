from typing import Annotated

from pycircuit import module


@module
def Child(x: Annotated[int, range(1 << 8)], *, N: int = 3) -> {"y": Annotated[int, range(1 << 8)]}:
    return {"y": x}
