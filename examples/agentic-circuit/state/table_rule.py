"""Minimal stateful rule: one Table replace and Queue transfer commit together."""

import agentic_circuit as ac


@ac.struct
class Entry:
    index: ac.u1
    value: ac.u7


@ac.rule
def install(rob, entry):
    old = rob[entry.index]
    rob[entry.index] = entry
    return old


@ac.system
def table_rule() -> None:
    rob = ac.table[2, Entry](init=0)
    incoming = ac.source(Entry, depth=2)
    outgoing = install(rob, incoming)
    ac.sink(outgoing)
