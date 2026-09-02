"""Epoch 0.4 slot -> match -> choose -> state patch prototype."""

import agentic_circuit as ac


@ac.struct
class Entry:
    valid: bool
    tag: ac.u8
    age: ac.u8


@ac.struct
class Request:
    tag: ac.u8


@ac.system
def table_issue_slot() -> None:
    requests = ac.source(Request, depth=2, latency=1)
    pending = ac.slot(requests)
    issue = ac.table[4, Entry](init=0)

    ready = issue.match(
        lambda entry: pending.valid
        and entry.valid
        and entry.tag == pending.value.tag
    )
    grant = issue.choose(
        ready,
        count=1,
        policy="min",
        key=lambda entry: entry.age,
    )

    issue.view(grant.index).patch(
        enable=pending.valid and grant.valid,
        valid=False,
    )
    pending.release(when=pending.valid and grant.valid)

    snapshots = issue.view(0).read(depth=1, latency=1)
    ac.sink(snapshots)
