from pycircuit import module, rule, struct, u8


@struct
class XzOut:
    y: u8


@rule
def capture(q, in_a):
    q = in_a  # noqa: F841 - hardware register write


@module
def XzValueModel(in_a: u8) -> XzOut:
    q: u8 = 0
    capture(q, in_a)
    return XzOut(y=q)
