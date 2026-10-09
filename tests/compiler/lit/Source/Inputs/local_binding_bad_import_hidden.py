from typing import Annotated

from pycircuit import module

from typing import Annotated as hidden

@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    hidden = a
    return {"result": a}
