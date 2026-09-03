"""Resident-entry Issue Table contract example.

This example does not persist global tag-ready state or re-query readiness for
an Entry allocated after its completion. It demonstrates Table writers,
selection, and allocation, not the cross-Queue lost-wakeup solution tracked by
issue #11.
"""

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
def issue() -> None:
    # Slots retain input requests until their corresponding operation succeeds.
    # This makes a full Issue Table backpressure allocation instead of dropping
    # the incoming Entry.
    wakeups = ac.source(Wakeup, depth=2, latency=1)
    allocations = ac.source(Entry, depth=2, latency=1)
    wakeup = ac.slot(wakeups)
    allocation = ac.slot(allocations)
    issue = ac.table[4, Entry](init=0)

    # Wakeup writers independently update the two readiness fields. Candidate
    # masks and value expressions read the old committed Table image.
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

    # Select the oldest entry whose operands were already ready at the start of
    # the tick. Wakeups performed above become visible to selection next tick.
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

    # Allocation explicitly finds a free slot; the Table primitive does not
    # search for one automatically. Since match reads old state, a slot cleared
    # by the issue writer this tick becomes allocatable on the following tick.
    empty = issue.match(lambda entry: allocation.valid and not entry.valid)
    free = issue.choose(empty, count=1, policy="first")
    issue.view(free.index).allocate(
        enable=free.valid,
        value=allocation.value,
    )

    # This resident-only example consumes a wakeup whenever it is present. A
    # complete implementation must persist readiness and re-query it when an
    # Entry arrives later; see issue #11. Consume an allocation only after a
    # free slot was selected and the complete replacement was proposed.
    wakeup.release(when=wakeup.valid)
    allocation.release(when=free.valid)
    ac.sink(output)
