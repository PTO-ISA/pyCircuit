import pycircuit as ac


@ac.module
def Top(a: ac.u3) -> ac.u16:  # noqa: N802
    q = (
        0x1232
        if a == 0
        else (
            0xAEE0
            if a == 1
            else (
                0x27D4
                if a == 2
                else (
                    0x5A0E
                    if a == 3
                    else (
                        0x2066
                        if a == 4
                        else 0x64CE if a == 5 else 0xC526 if a == 6 else 0x2F19
                    )
                )
            )
        )
    )
    return q
