import agentic_circuit as ac


@ac.struct
class Entry:
    sequence: ac.u4
    value: ac.u16
    done: bool


@ac.rule
def complete(entry):
    return entry.with_fields(done=True)


@ac.system
def rob() -> None:
    issued = ac.source(Entry, depth=4, latency=1)
    completed = complete(issued)
    retired = ac.reorder(completed, by=Entry.sequence, entries=8, start=0)
    ac.sink(retired)
