from pycircuit import module, rule, u8


@rule
def capture(r, in_x):
    r = in_x  # noqa: F841 - hardware register write


@module
def TraceLeaf(in_x: u8) -> {"out_y": u8}:
    r: u8 = 0
    capture(r, in_x)
    return {"out_y": r}
