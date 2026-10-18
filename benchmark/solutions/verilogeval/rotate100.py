import pycircuit as ac


@ac.rule
def rotate(
    saved: ac.bits[100], incoming: ac.bits[100], load: ac.u1, direction: ac.u2
) -> None:
    if load:
        saved = incoming
    elif direction == 1:
        saved = ac.concat(saved[:1], saved[1:])
    elif direction == 2:
        saved = ac.concat(saved[:99], saved[99:])


@ac.module
def Top(
    incoming: ac.bits[100], load: ac.u1, direction: ac.u2
) -> ac.bits[100]:  # noqa: N802
    saved: ac.bits[100] = 0
    rotate(saved, incoming, load, direction)
    return saved
