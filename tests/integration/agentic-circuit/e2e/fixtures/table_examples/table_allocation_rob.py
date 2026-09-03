"""E2E fixture for ROB completion, removal, and allocation."""

import agentic_circuit as ac


@ac.struct
class Entry:
    valid: bool
    done: bool
    result: ac.u16


@ac.struct
class Completion:
    index: ac.u2
    result: ac.u16


@ac.system
def table_allocation_rob() -> None:
    completions = ac.source(Completion)
    allocations = ac.source(Entry)
    completion = ac.slot(completions)
    allocation = ac.slot(allocations)
    rob = ac.table[4, Entry](init=0)

    rob.view(completion.value.index).patch(
        enable=completion.valid,
        done=True,
        result=completion.value.result,
    )
    rob.view(0).patch(enable=True, valid=False)
    rob.view(0).allocate(enable=allocation.valid, value=allocation.value)
    snapshot = rob.view(0).read()

    completion.release(when=completion.valid)
    allocation.release(when=allocation.valid)
    ac.sink(snapshot)
