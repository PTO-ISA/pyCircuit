"""E2E fixture for waking matching Issue Table entries."""

import agentic_circuit as ac


@ac.struct
class Entry:
    source_tag: ac.u8
    ready: bool


@ac.struct
class Completion:
    tag: ac.u8


@ac.system
def table_batch_wakeup() -> None:
    completions = ac.source(Completion, depth=2, latency=1)
    wakeup = ac.slot(completions)
    issue = ac.table[4, Entry](init=0)

    waiting = issue.match(lambda entry: entry.source_tag == wakeup.value.tag)
    issue.view(waiting).patch(enable=wakeup.valid, ready=True)
    wakeup.release(when=wakeup.valid)

    snapshots = issue.view(0).read(depth=1, latency=1)
    ac.sink(snapshots)
