from typing import Annotated
from pycircuit import module


@module
def Top(a: Annotated[int, range(1 << 13)]
        ) -> {"result": Annotated[int, range(1 << 13)]}:
    return {"result": missing(a)}
