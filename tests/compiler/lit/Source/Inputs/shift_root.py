from typing import Annotated

from pycircuit import module, rule


@module
def Word(a: Annotated[int, range(1 << 8)]) -> {"q": Annotated[int, range(1 << 8)]}:
    return {"q": a}


@module
def Shift(a: Annotated[int, range(1 << 8)],
          mid: Annotated[int, range(1 << 13)],
          addr: Annotated[int, range(1 << 40)],
          wide: Annotated[int, range(1 << 65)],
          one: Annotated[int, range(1 << 1)]) -> {
    "identity": Annotated[int, range(1 << 8)],
    "original_xor": Annotated[int, range(1 << 8)],
    "original_or": Annotated[int, range(1 << 8)],
    "original_equal": bool,
    "mid_zero": Annotated[int, range(1 << 13)],
    "mid_one": Annotated[int, range(1 << 12)],
    "mid_last": Annotated[int, range(1 << 1)],
    "mid_computed": Annotated[int, range(1 << 1)],
    "mid_at": Annotated[int, range(1 << 1)],
    "mid_above": Annotated[int, range(1 << 1)],
    "mid_huge": Annotated[int, range(1 << 1)],
    "tag": Annotated[int, range(1 << 28)],
    "wide_zero": Annotated[int, range(1 << 65)],
    "wide_one": Annotated[int, range(1 << 64)],
    "wide_last": Annotated[int, range(1 << 1)],
    "wide_at": Annotated[int, range(1 << 1)],
    "wide_above": Annotated[int, range(1 << 1)],
    "wide_huge": Annotated[int, range(1 << 1)],
    "one_zero": Annotated[int, range(1 << 1)],
    "one_at": Annotated[int, range(1 << 1)],
    "add_before": Annotated[int, range(1 << 1)],
    "masked": Annotated[int, range(1 << 8)],
    "singleton": Annotated[int, range(1 << 64)],
    "static_signed": Annotated[int, range(1 << 8)],
    "bound": Annotated[int, range(1 << 8)]}:
    child = Word()

    @rule
    def bind():
        child(a=(a + 1) >> 1)

    bind()
    return {
        "identity": a >> 0,
        "original_xor": a ^ 1,
        "original_or": a | 1,
        "original_equal": a == 0,
        "mid_zero": mid >> 0,
        "mid_one": mid >> 1,
        "mid_last": mid >> 12,
        "mid_computed": mid >> (6 + 6),
        "mid_at": mid >> 13,
        "mid_above": mid >> 14,
        "mid_huge": mid >> (1 << 300),
        "tag": addr >> 12,
        "wide_zero": wide >> 0,
        "wide_one": wide >> 1,
        "wide_last": wide >> 64,
        "wide_at": wide >> 65,
        "wide_above": wide >> 66,
        "wide_huge": wide >> (1 << 300),
        "one_zero": one >> 0,
        "one_at": one >> 1,
        "add_before": (a + 1) >> 8,
        "masked": (wide >> 1) & 255,
        "singleton": (wide * 0) >> 1,
        "static_signed": ((0 - 7) >> 1) & 255,
        "bound": child.q}
