from typing import Annotated
from pycircuit import dff, module, rule


@module
def Bad() -> {"result": bool}:
    return {"result": 1}
