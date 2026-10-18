import pycircuit as ac


@ac.struct
class Outputs:
    out: ac.u1
    out_n: ac.u1


@ac.module
def Top(a: ac.u1, b: ac.u1, c: ac.u1, d: ac.u1) -> Outputs:  # noqa: N802
    out = (a & b) | (c & d)
    return Outputs(out=out, out_n=~out)
