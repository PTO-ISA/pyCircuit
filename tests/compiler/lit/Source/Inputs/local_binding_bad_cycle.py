from typing import Annotated

from pycircuit import module, rule


@module
def Wire(a: Annotated[int, range(1 << 8)]) -> {"q": Annotated[int, range(1 << 8)]}:
    return {"q": a}


@module
def Bad() -> {"result": Annotated[int, range(1 << 8)]}:
    child = Wire()
    alias = child.q
    @rule
    def bind():
        child(a=alias)
    bind()
    return {"result": alias}
