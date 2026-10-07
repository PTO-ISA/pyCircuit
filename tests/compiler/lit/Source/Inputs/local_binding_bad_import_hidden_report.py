from typing import Annotated

from pycircuit import module

from pycircuit import report as hidden_report

@module
def Bad(a: Annotated[int, range(1 << 8)], flag: bool, *, COUNT: int = 1) -> {"result": Annotated[int, range(1 << 8)]}:
    hidden_report = a
    return {"result": a}
