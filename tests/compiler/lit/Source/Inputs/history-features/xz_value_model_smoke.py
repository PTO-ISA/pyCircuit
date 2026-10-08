from pycircuit import module, rule, u8


@rule
def capture(q, in_a):
    q = in_a  # noqa: F841 - hardware register write


@module
def XzValueModel(in_a: u8) -> {"y": u8}:
    q: u8 = 0
    capture(q, in_a)
    return {"y": q}
