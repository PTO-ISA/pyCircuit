# ruff: noqa: F823, F841 -- captured module-owned whole-Table proposals.
import pycircuit as ac


@ac.module
def TableCell(selector: ac.u1):
    state = ac.table[2, ac.u8](init=7)

    @ac.rule
    def update(first: ac.u8, second: ac.u8) -> None:
        state = (state[0] + first, state[1] + second)

    @ac.rule
    def value() -> ac.table[2, ac.u8]:
        return state

    @ac.rule
    def local_update() -> None:
        if not selector:
            state = (state[0] + 2, state[1] + 4)

    local_update()
