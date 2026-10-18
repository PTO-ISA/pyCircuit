import pycircuit as ac


@ac.struct
class Outputs:
    sum: ac.u1
    cout: ac.u1


@ac.module
def Top(a: ac.u1, b: ac.u1) -> Outputs:  # noqa: N802
    return Outputs(sum=a ^ b, cout=a & b)
