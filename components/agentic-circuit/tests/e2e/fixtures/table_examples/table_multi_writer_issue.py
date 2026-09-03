"""E2E fixture for independent Issue Table field writers."""

import agentic_circuit as ac


@ac.struct
class Entry:
    valid: bool
    age: ac.u8
    src0_tag: ac.u8
    src0_ready: bool
    src1_tag: ac.u8
    src1_ready: bool


@ac.struct
class Wakeup:
    tag: ac.u8
    valid: bool


@ac.system
def table_multi_writer_issue() -> None:
    wakeups = ac.source(Wakeup, depth=2, latency=1)
    wakeup = ac.slot(wakeups)
    issue = ac.table[4, Entry](init=0)

    src0_hits = issue.match(
        lambda entry: entry.valid
        and not entry.src0_ready
        and entry.src0_tag == wakeup.value.tag
    )
    issue.view(src0_hits).patch(
        enable=wakeup.valid,
        src0_ready=True,
    )

    src1_hits = issue.match(
        lambda entry: entry.valid
        and not entry.src1_ready
        and entry.src1_tag == wakeup.value.tag
    )
    issue.view(src1_hits).patch(enable=wakeup.valid, src1_ready=True)

    ready = issue.match(
        lambda entry: entry.valid and entry.src0_ready and entry.src1_ready
    )
    grant = issue.choose(
        ready,
        count=1,
        policy="min",
        key=lambda entry: entry.age,
    )
    output = issue.view(grant.index).read(
        when=grant.valid,
        depth=1,
        latency=1,
    )
    issue.view(grant.index).patch(enable=grant.valid, valid=False)

    wakeup.release(when=wakeup.valid)
    ac.sink(output)
