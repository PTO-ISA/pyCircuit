import pycircuit as ac


@ac.struct
class Outputs:
    out: ac.u8
    result_is_zero: ac.u1


@ac.module
def Top(do_sub: ac.u1, a: ac.u8, b: ac.u8) -> Outputs:  # noqa: N802
    out = a - b if do_sub else a + b
    return Outputs(out=out, result_is_zero=out == 0)
