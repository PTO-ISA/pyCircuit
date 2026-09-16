"""Design-neutral fixture for one firing's mixed-field Table write batch."""

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
def patch(entries, update):
    admitted = entries[update.index]
    entries[update.index] = admitted.with_fields(admitted=True)
    ready = entries[update.index]
    entries[update.index] = ready.with_fields(src_ready=True)


@ac.system
def table_rule_field_merge() -> None:
    entries = ac.table[4, Entry](init=0)
    updates = ac.source(Update, depth=2, latency=1)
    patch(entries, updates)
