from typing import Annotated
from pycircuit import module


@module
def Bad() -> {"result": Annotated[int, range(1 << 8)]}:
    return {"result": 256}
