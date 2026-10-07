"""pyc_credit_pipeline: hardware fixture; migration notes in tests/compiler/oracles/queue_source/MIGRATION-NOTES.md."""
# ruff: noqa: F821, N802 -- ordinary forward hardware queue-result wires.
import pycircuit as ac


@ac.struct
class CreditToken:
    sequence: ac.u4
    cycles: ac.u4
    value: ac.u16


@ac.struct
class CreditSlot:
    valid: ac.u1
    remaining: ac.u4
    data: CreditToken


@ac.struct
class CreditState:
    slot0: CreditSlot
    slot1: CreditSlot


@ac.struct
class CreditResult:
    ready: ac.u1
    available: ac.u1
    head: CreditToken


@ac.rule
def Advance(  # noqa: N802
    state,
    admit0,
    admit1,
    retire0,
    retire1,
    active0,
    active1,
    cost,
    token,
):
    """One edge of credit bookkeeping: retire / count down / admit, per slot.

    ``admit``/``retire``/``active`` are mutually exclusive within a slot
    (``free = ~valid`` cannot meet ``done = valid & remaining == 0``), so the
    ``if / elif`` chain is a total priority and needs no grant.  Admission writes
    ``cost`` over ``remaining`` (ST-03: no countdown on the admission edge), and a
    retirement clears ``valid`` only (ST-05: the credit is returned on the edge
    where the push into ``completed`` actually commits).
    """
    if admit0:
        state.slot0.valid = 1
        state.slot0.remaining = cost
        state.slot0.data = token
    elif retire0:
        state.slot0.valid = 0
    elif active0:
        state.slot0.remaining = state.slot0.remaining - 1
    if admit1:
        state.slot1.valid = 1
        state.slot1.remaining = cost
        state.slot1.data = token
    elif retire1:
        state.slot1.valid = 0
    elif active1:
        state.slot1.remaining = state.slot1.remaining - 1


@ac.module
def CreditPipeline(  # noqa: N802
    valid: ac.u1, data: CreditToken, take: ac.u1
) -> CreditResult:
    # All-zero reset means two empty slots.
    state: CreditState = CreditState()
    free0 = ~state.slot0.valid
    free1 = ~state.slot1.valid
    done0 = state.slot0.valid & (state.slot0.remaining == 0)
    done1 = state.slot1.valid & (state.slot1.remaining == 0)
    # `ready` below is the QUEUE CAPACITY (Q-D1(a)).  The third argument is the
    # credit-side admission condition (Q-D1(b), demoted): a free slot AND a
    # non-zero head cost.  Do not collapse the two -- see the T-03 note above.
    ready, available, head = ac.queue[CreditToken](
        valid,
        data,
        (free0 | free1) & ~(available & (head.cycles == 0)),
        depth=4,
        latency=1,
        ready_policy="local_occupancy",
    )
    admit = available & (free0 | free1) & ~(available & (head.cycles == 0))
    admit0 = admit & free0
    admit1 = admit & ~free0 & free1
    active0 = state.slot0.valid & ~done0
    active1 = state.slot1.valid & ~done1
    space, completed_valid, completed_head = ac.queue[CreditToken](
        done0 | done1,
        state.slot0.data if done0 else state.slot1.data,
        take,
        depth=4,
        latency=1,
        ready_policy="local_occupancy",
    )
    # Lowest-index tie-breaks, and retirement is gated by the push committing.
    retire0 = done0 & space
    retire1 = done1 & ~done0 & space
    Advance(
        state,
        admit0,
        admit1,
        retire0,
        retire1,
        active0,
        active1,
        head.cycles,
        head,
    )
    return CreditResult(ready=ready, available=completed_valid, head=completed_head)
