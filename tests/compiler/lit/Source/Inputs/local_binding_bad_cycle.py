from typing import Annotated

from pycircuit import module, rule


@module
def Wire(a: Annotated[int, range(1 << 8)]) -> {"q": Annotated[int, range(1 << 8)]}:
    return {"q": a}


@module
def Bad() -> {"result": Annotated[int, range(1 << 8)]}:
    child = Wire()
    alias = child.q
    child(a=alias)
    return {"result": alias}
