# ruff: noqa: N802, F823, F841 -- captured hardware declarations and state.
"""Complete finite state + pure hierarchy, with an external blocking read port."""

import pycircuit as ac
from clocked_accumulator.provider import increment


@ac.struct
class Input:
    ready: ac.u1
    data: ac.u32
    fault: ac.u1
    dready: ac.u1
    ddata: ac.u32
    dfault: ac.u1


@ac.struct
class State:
    words: ac.table[4, ac.u32]
    index: ac.u3
    total: ac.u32


@ac.struct
class Output:
    valid: ac.u1
    address: ac.u32
    write: ac.u1
    data: ac.u32
    mask: ac.u4
    completed: ac.u1
    done: ac.u1
    total: ac.u32


@ac.module
def Increment(value: ac.u32) -> ac.u32:
    @ac.rule
    def bump() -> ac.u32:
        return increment(value)

    return bump


@ac.module(execution={"profile": "clocked_fixedpoint_v1", "state_output": 1})
def Accumulator(incoming: Input) -> tuple[Output, State]:
    state: State = State()
    value = Increment(incoming.data)

    @ac.rule
    def advance() -> Output:
        address: ac.u32 = state.index
        io = Output(
            valid=state.index < 4,
            address=address << 2,
            write=0,
            data=0,
            mask=15,
            completed=0,
            done=0,
            total=state.total,
        )
        if state.index < 4 and incoming.ready:
            state.words[state.index[0:2]] = value
            state.total = state.total + value
            state.index = state.index + 1
            io.completed = 1
            io.done = state.index == 4
            io.total = state.total
        return io

    return advance, state
