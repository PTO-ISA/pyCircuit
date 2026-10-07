from typing import Annotated
from pycircuit import module, rule
from numeric.numeric_mask import Computed, Mask


@module
def MaskTop(a: Annotated[int, range(1 << 8)],
            c: Annotated[int, range(1 << (4 + 4))],
            d: Annotated[int, range(1 << (4 + 4))],
            integer_input: Annotated[int, range(1 << 1)],
            left_flag: bool, right_flag: bool
            ) -> {"default_left": Annotated[int, range(1 << 8)],
                  "default_right": Annotated[int, range(1 << 8)],
                  "fifteen_left": Annotated[int, range(1 << 8)],
                  "fifteen_right": Annotated[int, range(1 << 8)],
                  "alternate_left": Annotated[int, range(1 << 8)],
                  "alternate_right": Annotated[int, range(1 << 8)],
                  "computed": Annotated[int, range(1 << (4 + 4))],
                  "computed_left": Annotated[int, range(1 << (4 + 4))],
                  "computed_right": Annotated[int, range(1 << (4 + 4))],
                  "signed_constant": Annotated[int, range(1 << 8)],
                  "integer_bit": Annotated[int, range(1 << 1)],
                  "boolean_bit": bool, "integer_equal": bool,
                  "boolean_equal": bool}:
    default = Mask()
    fifteen = Mask(MASK=15)
    alternate = Mask(MASK=170)
    computed = Computed()

    @rule
    def bind_default():
        default(a=a)

    @rule
    def bind_fifteen():
        fifteen(a=a)

    @rule
    def bind_alternate():
        alternate(a=a)

    @rule
    def bind_computed():
        computed(a=c, b=d)

    bind_default()
    bind_fifteen()
    bind_alternate()
    bind_computed()
    return {"default_left": default.left, "default_right": default.right,
            "fifteen_left": fifteen.left, "fifteen_right": fifteen.right,
            "alternate_left": alternate.left, "alternate_right": alternate.right,
            "computed": computed.q, "computed_left": computed.fifteen_left,
            "computed_right": computed.fifteen_right,
            "signed_constant": (0 - 1) & 255,
            "integer_bit": integer_input ^ 1,
            "boolean_bit": left_flag ^ right_flag,
            "integer_equal": integer_input == 1,
            "boolean_equal": left_flag == right_flag}
