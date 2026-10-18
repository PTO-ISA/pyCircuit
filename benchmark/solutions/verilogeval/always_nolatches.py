import pycircuit as ac


@ac.struct
class Outputs:
    left: ac.u1
    down: ac.u1
    right: ac.u1
    up: ac.u1


@ac.module
def Top(scancode: ac.u16) -> Outputs:  # noqa: N802
    return Outputs(
        left=scancode == 0xE06B,
        down=scancode == 0xE072,
        right=scancode == 0xE074,
        up=scancode == 0xE075,
    )
