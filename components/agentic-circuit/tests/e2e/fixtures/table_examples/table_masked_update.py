"""E2E fixture for CandidateSet-driven masked Table updates."""

import agentic_circuit as ac


@ac.struct
class Entry:
    valid: bool
    tag: ac.u8
    age: ac.u8


@ac.struct
class Update:
    tag: ac.u8


@ac.system
def table_masked_update() -> None:
    updates = ac.source(Update, depth=2, latency=1)
    pending = ac.slot(updates)
    issue = ac.table[4, Entry](init=0)

    invalid = issue.match(lambda entry: not entry.valid)
    issue.view(invalid).patch(
        enable=pending.valid,
        valid=True,
        tag=pending.value.tag,
        age=lambda entry: entry.age + 1,
    )
    pending.release(when=pending.valid)

    snapshots = issue.view(0).read(depth=1, latency=1)
    ac.sink(snapshots)
