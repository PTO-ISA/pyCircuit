import pycircuit as ac


@ac.struct
class Outputs:
    out: ac.u4
    valid: ac.u1


@ac.module
def Top(code: ac.u8) -> Outputs:  # noqa: N802
    valid = (
        (code == 0x45)
        | (code == 0x16)
        | (code == 0x1E)
        | (code == 0x26)
        | (code == 0x25)
        | (code == 0x2E)
        | (code == 0x36)
        | (code == 0x3D)
        | (code == 0x3E)
        | (code == 0x46)
    )  # noqa: E501
    out = (
        0
        if code == 0x45
        else (
            1
            if code == 0x16
            else (
                2
                if code == 0x1E
                else (
                    3
                    if code == 0x26
                    else (
                        4
                        if code == 0x25
                        else (
                            5
                            if code == 0x2E
                            else (
                                6
                                if code == 0x36
                                else (
                                    7
                                    if code == 0x3D
                                    else 8 if code == 0x3E else 9 if code == 0x46 else 0
                                )
                            )
                        )
                    )
                )
            )
        )
    )  # noqa: E501
    return Outputs(out=out, valid=valid)
