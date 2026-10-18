import pycircuit as ac


@ac.struct
class Outputs:
    outv: ac.u3
    o2: ac.u1
    o1: ac.u1
    o0: ac.u1


@ac.module
def Top(vec: ac.u3) -> Outputs:  # noqa: N802
    return Outputs(outv=vec, o2=vec[2:3], o1=vec[1:2], o0=vec[:1])
