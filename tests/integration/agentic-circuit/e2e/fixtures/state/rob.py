"""A bounded, non-wrapping retirement demo using the rule-oriented surface.

The rule describes only the payload update.  Queue consumption, output
handshake, scheduling, and atomic transfer are inferred and materialized by
the ACIR rule pipeline before the model reaches gfsim.
"""

import agentic_circuit as ac


@ac.struct
class Entry:
    sequence: ac.u4
    value: ac.u16
    done: bool


@ac.rule
def complete(entry):
    return entry.with_fields(done=True)


@ac.system
def rob() -> None:
    issued = ac.source(Entry, depth=4, latency=1)
    completed = complete(issued)
    retired = ac.reorder(
        completed,
        by=Entry.sequence,
        entries=8,
        start=0,
        depth=2,
        latency=1,
    )
    ac.sink(retired)


specialization = ac.jit(rob)
