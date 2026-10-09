from typing import Annotated
from pycircuit import module


@module
def Numeric(a: Annotated[int, range(1 << 13)],
            b: Annotated[int, range(1 << 13)],
            wide_a: Annotated[int, range(1 << 65)],
            wide_b: Annotated[int, range(1 << 65)],
            computed: Annotated[int, range(1 << (3 + 2))]
            ) -> {"sum14": Annotated[int, range(1 << 14)],
                  "masked13": Annotated[int, range(1 << 13)],
                  "swapped13": Annotated[int, range(1 << 13)],
                  "nonfull8": Annotated[int, range(1 << 8)],
                  "zero1": Annotated[int, range(1 << 1)],
                  "product26": Annotated[int, range(1 << 26)],
                  "delta13": Annotated[int, range(1 << 13)],
                  "nested13": Annotated[int, range(1 << 13)],
                  "wide_sum66": Annotated[int, range(1 << 66)],
                  "wide_low8": Annotated[int, range(1 << 8)],
                  "wide_product130": Annotated[int, range(1 << 130)],
                  "wide_zero1": Annotated[int, range(1 << 1)],
                  "computed6": Annotated[int, range(1 << 6)]}:
    return {"sum14": a + b, "masked13": (a + b) & 8191,
            "swapped13": 8191 & (a + b), "nonfull8": (a + b) & 171,
            "zero1": (a + b) & 0, "product26": a * b,
            "delta13": (a - b) & 8191,
            "nested13": ((a - b) * (a + b)) & 8191,
            "wide_sum66": wide_a + wide_b,
            "wide_low8": (wide_a + wide_b) & 255,
            "wide_product130": wide_a * wide_b,
            "wide_zero1": (wide_a + wide_b) & 0,
            "computed6": computed + 1}
