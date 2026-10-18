import pycircuit as ac


@ac.module
def Top(in_: ac.u8) -> ac.u8:  # noqa: N802
    return ac.concat(
        in_[0:1],
        in_[1:2],
        in_[2:3],
        in_[3:4],
        in_[4:5],
        in_[5:6],
        in_[6:7],
        in_[7:8],
    )  # noqa: E501
