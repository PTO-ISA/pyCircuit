from typing import Annotated
from pycircuit import module, rule
from computed.computed_child import Child


@module
def Top(a13: Annotated[int, range(1 << 13)],
        b13: Annotated[int, range(1 << 13)],
        a37: Annotated[int, range(1 << 37)],
        b37: Annotated[int, range(1 << 37)], choose: bool
        ) -> {"masked13": Annotated[int, range(1 << 13)],
              "xor37": Annotated[int, range(1 << 37)],
              "mixed13": Annotated[int, range(1 << 13)],
              "pick37": Annotated[int, range(1 << 37)],
              "less": bool, "truth": bool,
              "zero13": Annotated[int, range(1 << 13)]}:
    child = Child()

    @rule
    def bind():
        child(x13=b13, x37=b37)

    bind()
    return {"masked13": a13 & b13, "xor37": a37 ^ b37,
            "mixed13": a13 | child.t13,
            "pick37": child.t37 if choose else a37,
            "less": a13 < b13, "truth": True, "zero13": 0}
