import pycircuit as ac


@ac.struct
class Outputs:
    out_hi: ac.u8
    out_lo: ac.u8


@ac.module
def Top(in_: ac.u16) -> Outputs:  # noqa: N802
    return Outputs(out_hi=in_[8:16], out_lo=in_[:8])
