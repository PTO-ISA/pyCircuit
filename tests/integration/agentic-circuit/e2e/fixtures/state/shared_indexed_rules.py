"""Two lexical rules sharing one explicit Table."""

import agentic_circuit as ac


@ac.struct
class Entry:
    index: ac.u2
    value: ac.u8


@ac.rule
def replace(entries, incoming):
    old = entries[incoming.index]
    entries[incoming.index] = incoming
    return old


@ac.system
def shared_indexed_rules(first: Entry, second: Entry) -> tuple[Entry, Entry]:
    entries = ac.table[4, Entry](init=0)
    first_old = replace(entries, first)
    second_old = replace(entries, second)
    return first_old, second_old
