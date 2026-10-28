from typing import Annotated
from pycircuit import module
from selects.select_parameter import Parameter


@module
def Parameters(a: Annotated[int, range(1 << 8)],
               b: Annotated[int, range(1 << 12)], flag: bool
               ) -> {"default_right": Annotated[int, range(1 << 8)],
                     "default_left": Annotated[int, range(1 << 8)],
                     "override_right": Annotated[int, range(1 << 8)],
                     "override_left": Annotated[int, range(1 << 8)],
                     "wide_right": Annotated[int, range(1 << 12)],
                     "wide_left": Annotated[int, range(1 << 12)]}:
    default = Parameter()
    override = Parameter(VALUE=15)
    wide = Parameter(WIDTH=12, VALUE=1023)

    default(a=a, flag=flag)

    override(a=a, flag=flag)

    wide(a=b, flag=flag)

    return {"default_right": default.right, "default_left": default.left,
            "override_right": override.right, "override_left": override.left,
            "wide_right": wide.right, "wide_left": wide.left}
