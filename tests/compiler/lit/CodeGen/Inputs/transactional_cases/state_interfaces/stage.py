# ruff: noqa: F823 -- Module state is compiler-owned.
import pycircuit as ac


@ac.module
def Stage(source: ac.u8) -> ac.u8:  # noqa: N802
    accepted: ac.u8 = 0

    @ac.rule(guard=lambda: accepted < 2)
    def advance(value: ac.u8) -> ac.u8:
        accepted = accepted + 1
        return value + accepted

    result = advance(source)

    return result


@ac.module
def Export(source: ac.u8) -> ac.u8:  # noqa: N802
    return source
