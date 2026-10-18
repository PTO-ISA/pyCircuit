import pycircuit as ac


@ac.struct
class Outputs:
    out_assign: ac.u1
    out_if: ac.u1


@ac.module
def Top(a: ac.u1, b: ac.u1, sel_b1: ac.u1, sel_b2: ac.u1) -> Outputs:  # noqa: N802
    out = b if sel_b1 & sel_b2 else a
    return Outputs(out_assign=out, out_if=out)
