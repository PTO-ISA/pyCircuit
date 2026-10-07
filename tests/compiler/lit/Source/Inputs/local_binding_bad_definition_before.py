from typing import Annotated

from pycircuit import module


@module
def Word(a: Annotated[int, range(1 << 8)]) -> {"q": Annotated[int, range(1 << 8)]}:
    return {"q": a}


@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    Word = a
    return {"result": a}
