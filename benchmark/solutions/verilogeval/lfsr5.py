import pycircuit as ac


@ac.rule
def advance(sequence: ac.u5) -> None:
    feedback = sequence[:1]
    mask: ac.u5 = 20 if feedback else 0
    sequence = (sequence >> 1) ^ mask


@ac.module
def Top() -> ac.u5:  # noqa: N802
    sequence: ac.u5 = 1
    advance(sequence)
    return sequence
