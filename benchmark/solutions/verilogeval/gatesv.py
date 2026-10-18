import pycircuit as ac


@ac.struct
class Outputs:
    out_both: ac.u3
    out_any: ac.u3
    out_different: ac.u4


@ac.module
def Top(in_: ac.u4) -> Outputs:  # noqa: N802
    adjacent_left = in_[1:4]
    adjacent_right = in_[:3]
    wrapped = ac.concat(in_[3:4] ^ in_[:1], adjacent_left ^ adjacent_right)
    return Outputs(
        out_both=adjacent_left & adjacent_right,
        out_any=adjacent_left | adjacent_right,
        out_different=wrapped,
    )
