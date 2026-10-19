import pycircuit as ac


@ac.struct
class Outputs:
    next_state: ac.u2
    out: ac.u1


@ac.module
def Top(in_: ac.u1, state: ac.u2) -> Outputs:  # noqa: N802
    if state == 0:
        next_state = 1 if in_ else 0
    elif state == 1:
        next_state = 1 if in_ else 2
    elif state == 2:
        next_state = 3 if in_ else 0
    else:
        next_state = 1 if in_ else 2
    return Outputs(next_state=next_state, out=state == 3)
