"""A completion port with an inferred consume-only atomic state rule."""

import agentic_circuit as ac


@ac.struct
class Entry:
    index: ac.u2
    generation: ac.u8
    value: ac.u8


@ac.rule
def complete(entries, completion):
    old = entries[completion.index]
    if old.generation != completion.generation:
        return
    entries[completion.index] = completion


@ac.system
def consume_only_completion(completion: Entry) -> None:
    entries = ac.table[4, Entry](init=0)
    complete(entries, completion)
