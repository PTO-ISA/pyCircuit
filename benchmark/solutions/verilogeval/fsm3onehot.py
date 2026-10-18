import pycircuit as ac


@ac.struct
class Outputs:
    next_state: ac.u4
    out: ac.u1


@ac.module
def Top(in_: ac.u1, state: ac.u4) -> Outputs:  # noqa: N802
    a = state[:1]
    b = state[1:2]
    c = state[2:3]
    d = state[3:4]
    next_a = ~in_ & (a | c)
    next_b = in_ & (a | b | d)
    next_c = ~in_ & (b | d)
    next_d = in_ & c
    return Outputs(next_state=ac.concat(next_d, next_c, next_b, next_a), out=d)
