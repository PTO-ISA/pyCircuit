import pycircuit as ac


@ac.struct
class Outputs:
    w: ac.u1
    x: ac.u1
    y: ac.u1
    z: ac.u1


@ac.module
def Top(a: ac.u1, b: ac.u1, c: ac.u1) -> Outputs:  # noqa: N802
    return Outputs(w=a, x=b, y=b, z=c)
