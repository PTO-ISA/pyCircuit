from typing import Annotated

from pycircuit import module


@module
def Word(a: Annotated[int, range(1 << 8)]) -> {"q": Annotated[int, range(1 << 8)]}:
    return {"q": a}
