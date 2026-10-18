import pycircuit as ac


@ac.rule
def advance(sequence: ac.u32) -> None:
    feedback = sequence[:1]
    mask: ac.u32 = 0x80200003 if feedback else 0
    sequence = (sequence >> 1) ^ mask


@ac.module
def Top() -> ac.u32:  # noqa: N802
    sequence: ac.u32 = 1
    advance(sequence)
    return sequence
