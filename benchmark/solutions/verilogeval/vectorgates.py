import pycircuit as ac


@ac.struct
class Outputs:
    out_or_bitwise: ac.u3
    out_or_logical: ac.u1
    out_not: ac.u6


@ac.module
def Top(a: ac.u3, b: ac.u3) -> Outputs:  # noqa: N802
    return Outputs(
        out_or_bitwise=a | b,
        out_or_logical=(a != 0) | (b != 0),
        out_not=ac.concat(~b, ~a),
    )
