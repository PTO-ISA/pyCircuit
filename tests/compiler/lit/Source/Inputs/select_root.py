from typing import Annotated
from pycircuit import module


@module
def Select(a: Annotated[int, range(1 << 8)],
           b: Annotated[int, range(1 << 8)],
           narrow: Annotated[int, range(1 << 5)],
           wide: Annotated[int, range(1 << 65)],
           flag: bool, left: bool, right: bool
           ) -> {"constant_right": Annotated[int, range(1 << 10)],
                 "constant_left": Annotated[int, range(1 << 10)],
                 "masked_right": Annotated[int, range(1 << 8)],
                 "masked_left": Annotated[int, range(1 << 8)],
                 "raw_right": Annotated[int, range(1 << 65)],
                 "raw_left": Annotated[int, range(1 << 65)],
                 "after_select": Annotated[int, range(1 << 8)],
                 "narrow_selected": Annotated[int, range(1 << 8)],
                 "selected_signed": Annotated[int, range(1 << 8)],
                 "after_constant_right": Annotated[int, range(1 << 8)],
                 "after_constant_left": Annotated[int, range(1 << 8)],
                 "literal_true": Annotated[int, range(1 << 9)],
                 "literal_false": Annotated[int, range(1 << 9)],
                 "boolean_true": bool, "boolean_false": bool,
                 "boolean_dynamic": bool,
                 "nested_right": Annotated[int, range(1 << 8)],
                 "nested_left": Annotated[int, range(1 << 8)]}:
    return {"constant_right": (a + 1) if flag else 1023,
            "constant_left": 1023 if flag else (a + 1),
            "masked_right": (((a + 1) if flag else 1023) & 255),
            "masked_left": ((1023 if flag else (a + 1)) & 255),
            "raw_right": wide if flag else a,
            "raw_left": a if flag else wide,
            "after_select": ((wide if flag else a) + 1) & 255,
            "narrow_selected": a if flag else narrow,
            "selected_signed": ((a - b) if flag else 255) & 255,
            "after_constant_right": ((a if flag else 1023) + 1) & 255,
            "after_constant_left": ((1023 if flag else a) + 1) & 255,
            "literal_true": (a + 1) if True else b,
            "literal_false": (a + 1) if False else b,
            "boolean_true": left if True else right,
            "boolean_false": left if False else right,
            "boolean_dynamic": True if flag else False,
            "nested_right": ((((a + 1) if flag else 1023)
                               if left else (4095 if right else b)) & 255),
            "nested_left": (((4095 if right else b)
                              if left else ((a + 1) if flag else 1023)) & 255)}
