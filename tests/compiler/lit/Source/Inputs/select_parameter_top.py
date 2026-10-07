from typing import Annotated
from pycircuit import module, rule
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

    @rule
    def bind_default():
        default(a=a, flag=flag)

    @rule
    def bind_override():
        override(a=a, flag=flag)

    @rule
    def bind_wide():
        wide(a=b, flag=flag)

    bind_default()
    bind_override()
    bind_wide()
    return {"default_right": default.right, "default_left": default.left,
            "override_right": override.right, "override_left": override.left,
            "wide_right": wide.right, "wide_left": wide.left}
