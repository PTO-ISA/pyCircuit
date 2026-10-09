from history_features.trace_leaf import TraceLeaf
from pycircuit import module, rule, u8


@module
def TraceDsl(in_x: u8) -> {"y0": u8, "y1": u8}:
    unit0 = TraceLeaf()
    unit1 = TraceLeaf()

    @rule
    def drive0():
        unit0(in_x=in_x)

    @rule
    def drive1():
        unit1(in_x=in_x)

    drive0()
    drive1()
    return {"y0": unit0.out_y, "y1": unit1.out_y}
