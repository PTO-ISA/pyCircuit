import pycircuit as ac


@ac.module
def Top(in_: ac.u8) -> ac.u32:  # noqa: N802
    sign = in_[7:8]
    extended = ac.concat(
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        sign,
        in_,
    )
    return extended
