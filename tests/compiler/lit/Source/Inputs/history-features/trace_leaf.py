from pycircuit import module, rule, struct, u8


@struct
class LeafOut:
    out_y: u8


@rule
def capture(r, in_x):
    r = in_x  # noqa: F841 - hardware register write


@module
def TraceLeaf(in_x: u8) -> LeafOut:
    r: u8 = 0
    capture(r, in_x)
    return LeafOut(out_y=r)
