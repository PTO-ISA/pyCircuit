from typing import Annotated
from pycircuit import module


@module
def Child(x: Annotated[int, range(1 << 13)]
          ) -> {"q": Annotated[int, range(1 << 13)]}:
    return {"q": x}
