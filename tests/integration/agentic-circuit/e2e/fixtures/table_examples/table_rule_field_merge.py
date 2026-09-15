"""Design-neutral fixture for atomic disjoint-field Table rule writes."""

import agentic_circuit as ac


@ac.struct
class Entry:
    admitted: ac.u1
    src_ready: ac.u1
    tag: ac.u4


@ac.struct
class Update:
    index: ac.u2


@ac.rule
def admit(entries, update):
    entries[update.index].admitted = True


@ac.rule
def wake(entries, update):
    entries[update.index].src_ready = True


@ac.system
def table_rule_field_merge() -> None:
    entries = ac.table[4, Entry](init=0)
    updates = ac.source(Update, depth=2, latency=1)
    admit(entries, updates)
    wake(entries, updates)
