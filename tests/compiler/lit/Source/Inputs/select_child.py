from typing import Annotated
from pycircuit import module


@module
def Condition(flag: bool) -> {"q": bool}:
    return {"q": flag}


@module
def Word(a: Annotated[int, range(1 << 8)]
         ) -> {"q": Annotated[int, range(1 << 8)]}:
    return {"q": a}
