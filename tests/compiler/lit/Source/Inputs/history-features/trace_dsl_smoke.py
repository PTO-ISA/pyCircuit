from history_features.trace_leaf import TraceLeaf
from pycircuit import module, struct, u8


@struct
class DslOut:
    y0: u8
    y1: u8


@module
def TraceDsl(in_x: u8) -> DslOut:
    unit0 = TraceLeaf(in_x=in_x)
    unit1 = TraceLeaf(in_x=in_x)
    return DslOut(y0=unit0.out_y, y1=unit1.out_y)
