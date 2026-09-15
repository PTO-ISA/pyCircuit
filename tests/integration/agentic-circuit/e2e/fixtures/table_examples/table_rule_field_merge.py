"""E2E fixture for two @ac.rule field writers merging disjoint Entry fields.

Each rule reads the committed Entry and updates exactly one field through the
direct field spelling; both proposals narrow to `mode "field"`, so the two
rules may commit in one tick and the unwritten `tag` field keeps its committed
value.
"""

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
    old = entries[update.index]
    entries[update.index].admitted = True
    return old


@ac.rule
def wake(entries, update):
    old = entries[update.index]
    entries[update.index].src_ready = True
    return old


@ac.system
def table_rule_field_merge() -> None:
    entries = ac.table[4, Entry](init=0)
    updates = ac.source(Update, depth=2, latency=1)
    admitted = admit(entries, updates)
    woken = wake(entries, updates)
    ac.sink(admitted)
    ac.sink(woken)
