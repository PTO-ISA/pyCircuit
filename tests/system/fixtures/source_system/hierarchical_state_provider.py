# ruff: noqa: F823, F841 -- captured module-owned state proposals.
from enum import Enum

import pycircuit as ac


@ac.module
def StateCell(selector: ac.u1):
    state: ac.u8 = 7

    @ac.rule
    def update(delta: ac.u8) -> tuple[ac.u8, ac.u8]:
        before = state
        state = state + delta
        return before, state

    @ac.rule
    def value() -> ac.u8:
        return state

    @ac.rule
    def local_update() -> None:
        if not selector:
            state = state + 1

    local_update()


@ac.encoding(width=3)
class StateTag(Enum):
    ZERO = 0
    FIVE = 5


@ac.struct
class StatePayload:
    data: ac.u8
    tag: StateTag


@ac.module
def PayloadCell(selector: ac.u1):
    state: StatePayload = StatePayload(data=7, tag=StateTag.ZERO)

    @ac.rule
    def update(delta: ac.u8) -> None:
        state = StatePayload(
            data=state.data + delta,
            tag=StateTag.FIVE if state.tag == StateTag.ZERO else StateTag.ZERO,
        )

    @ac.rule
    def value() -> StatePayload:
        return state

    @ac.rule
    def local_update() -> None:
        if not selector:
            state = StatePayload(
                data=state.data + 2,
                tag=StateTag.FIVE if state.tag == StateTag.ZERO else StateTag.ZERO,
            )

    local_update()


@ac.module
def PayloadRelay(selector: ac.u1):
    child = PayloadCell(selector)

    @ac.rule
    def update(delta: ac.u8) -> None:
        child.update(delta)

    @ac.rule
    def value() -> StatePayload:
        return child.value()


@ac.module
def OrderedStateCell():
    state: ac.u8 = 7

    @ac.rule
    def update(delta: ac.u8) -> tuple[ac.u8, ac.u8]:
        before = state
        state = state + delta
        return before, state

    @ac.rule
    def value() -> ac.u8:
        return state
