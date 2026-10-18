import pycircuit as ac


@ac.struct
class Outputs:
    ringer: ac.u1
    motor: ac.u1


@ac.module
def Top(ring: ac.u1, vibrate_mode: ac.u1) -> Outputs:  # noqa: N802
    return Outputs(
        ringer=ring & ~vibrate_mode,
        motor=ring & vibrate_mode,
    )
