from typing import Annotated
from pycircuit import module


@module
def Child(x13: Annotated[int, range(1 << 13)],
          x37: Annotated[int, range(1 << 37)]
          ) -> {"t13": Annotated[int, range(1 << 13)],
                "t37": Annotated[int, range(1 << 37)]}:
    return {"t13": x13 ^ 8191, "t37": x37 & 137438953471}
