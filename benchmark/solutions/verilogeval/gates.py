import pycircuit as ac


@ac.struct
class Outputs:
    out_and: ac.u1
    out_or: ac.u1
    out_xor: ac.u1
    out_nand: ac.u1
    out_nor: ac.u1
    out_xnor: ac.u1
    out_anotb: ac.u1


@ac.module
def Top(a: ac.u1, b: ac.u1) -> Outputs:  # noqa: N802
    return Outputs(
        out_and=a & b,
        out_or=a | b,
        out_xor=a ^ b,
        out_nand=~(a & b),
        out_nor=~(a | b),
        out_xnor=~(a ^ b),
        out_anotb=a & ~b,
    )
