import pycircuit as ac


@ac.struct
class Outputs:
    out_and: ac.u1
    out_or: ac.u1
    out_xor: ac.u1


@ac.module
def Top(in_: ac.u4) -> Outputs:  # noqa: N802
    b0 = in_[0:1]
    b1 = in_[1:2]
    b2 = in_[2:3]
    b3 = in_[3:4]
    return Outputs(
        out_and=b0 & b1 & b2 & b3,
        out_or=b0 | b1 | b2 | b3,
        out_xor=b0 ^ b1 ^ b2 ^ b3,
    )
