import pycircuit as ac


@ac.struct
class Outputs:
    out_assign: ac.u1
    out_alwaysblock: ac.u1


@ac.module
def Top(a: ac.u1, b: ac.u1) -> Outputs:  # noqa: N802
    value = a & b
    return Outputs(out_assign=value, out_alwaysblock=value)
