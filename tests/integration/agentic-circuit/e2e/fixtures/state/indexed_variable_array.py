"""Explicit indexed Table state selected by MLIR storage lowering."""

import agentic_circuit as ac


@ac.struct
class Entry:
    index: ac.u3
    value: ac.u8


@ac.rule
def replace(entries, incoming):
    checked = ac.checked(incoming.index, ac.index[5])
    if checked.valid:
        old = entries[checked.value]
        entries[checked.value] = incoming
        return old


@ac.system
def indexed_variable_array(incoming: Entry) -> Entry:
    entries = ac.table[5, Entry](init=0)
    outgoing = replace(entries, incoming)
    return outgoing
