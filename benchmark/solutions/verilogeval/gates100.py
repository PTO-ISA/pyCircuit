import pycircuit as ac


@ac.struct
class Outputs:
    out_and: ac.u1
    out_or: ac.u1
    out_xor: ac.u1


@ac.module
def Top(in_: ac.bits[100]) -> Outputs:  # noqa: N802
    count = ac.popcount(in_)
    return Outputs(out_and=count == 100, out_or=count != 0, out_xor=count[:1])
