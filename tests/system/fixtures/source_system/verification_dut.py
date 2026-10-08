from pycircuit import module, rule, u8


@rule
def checked_advance(value):
    value = value + 1
    assert value < 7, "child limit seven"
    assert value < 8, "child limit eight"


@module
def CheckedChild() -> {"value": u8}:
    value: u8 = 0
    checked_advance(value)
    return {"value": value}
