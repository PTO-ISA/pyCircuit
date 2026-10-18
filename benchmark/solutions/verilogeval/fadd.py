import pycircuit as ac


@ac.struct
class Outputs:
    cout: ac.u1
    sum: ac.u1


@ac.module
def Top(a: ac.u1, b: ac.u1, cin: ac.u1) -> Outputs:  # noqa: N802
    return Outputs(
        cout=(a & b) | (a & cin) | (b & cin),
        sum=a ^ b ^ cin,
    )
