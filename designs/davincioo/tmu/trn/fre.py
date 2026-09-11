"""TMU.TRN.FRE — free physical tile slots and cancellable reservations.

FRE is the allocator. It hands out physical tile slots, keeps a repeated rename
transaction idempotent, and releases a slot only when the coordinator presents
both preconditions for a free. It holds no payload bytes, no descriptor and no
logical name: those belong to BANK, STS and RAT.

One rule serves all four mutating operations, which makes the allocator
single-port: a rule admits one token per tick, so at most one operation reaches
the pool per cycle. A second read-only rule answers capacity queries over the
same pool without ever writing it.

The Local pool is instantiated per PE, so one PE running out cannot touch
another's slots -- that isolation is structural, not a checked invariant. The
Shared pool is the same system instantiated once per core, because a core-owned
pool written by four per-PE rules would give one Table four write endpoints.

See designs/davincioo/contracts/tmu_trn.py for why the operations share one
stream, why a slot stores ``allocated`` rather than ``free``, and why responses
carry ``ac.find``'s index.
"""

from __future__ import annotations

import agentic_circuit as ac

from designs.davincioo.contracts.tmu_trn import (
    FRE_ALLOC,
    FRE_CANCEL,
    FRE_COMMIT,
    FRE_FREE,
    OPERATION_BRANCH_MASK,
    RESERVATION_SLOTS,
    TILE_SLOTS_PER_POOL,
    AllocRequest,
    CapacityQuery,
    Reservation,
    TileSlot,
)


@ac.rule
def serve_allocation(slots, reservations, request) -> AllocRequest:
    """Allocate, commit, cancel or free one physical tile slot.

    Four searches locate the work. ``existing`` makes a repeated allocate return
    the reservation it already has instead of a second slot. ``free_slot`` and
    ``free_row`` are the two resources an allocate needs at once, so requiring
    both is what makes allocation all-or-none. ``target`` finds a committed slot
    by owner and version, which is how a free arrives: the reclaim chain names a
    version, never a slot.
    """
    existing = ac.find(
        reservations,
        where=lambda entry: entry.live and entry.txn_key == request.txn_key,
        key=lambda entry: entry.slot_index,
    )
    free_slot = ac.find(
        slots,
        where=lambda entry: not entry.allocated,
        key=lambda entry: entry.tile_version,
    )
    free_row = ac.find(
        reservations,
        where=lambda entry: not entry.live,
        key=lambda entry: entry.slot_index,
    )
    target = ac.find(
        slots,
        where=lambda entry: (
            entry.allocated
            and entry.owner == request.owner
            and entry.tile_version == request.tile_version
        ),
        key=lambda entry: entry.tile_version,
    )

    is_alloc = request.operation == FRE_ALLOC
    is_commit = request.operation == FRE_COMMIT
    is_cancel = request.operation == FRE_CANCEL
    is_free = request.operation == FRE_FREE

    # An allocate needs a slot and a reservation row together, so a pool with
    # slots but no free row is exhausted for allocation purposes.
    has_capacity = free_slot.valid and free_row.valid
    allocates = is_alloc and not existing.valid and has_capacity

    # Commit and cancel both retire the reservation row. Only cancel gives the
    # slot back: after a commit the version is live and only a free may release
    # it.
    commits = is_commit and existing.valid
    cancels = is_cancel and existing.valid

    # A free requires both preconditions. FRE cannot observe REF's count or the
    # logical lifetime, so it refuses rather than assumes.
    frees = (
        is_free and target.valid and request.logical_permission and request.refs_zero
    )

    if allocates:
        slots[free_slot.index] = TileSlot(
            owner=request.owner,
            tile_version=request.tile_version,
            allocation_generation=request.allocation_generation,
            allocated=True,
        )
        reservations[free_row.index] = Reservation(
            txn_key=request.txn_key,
            owner=request.owner,
            slot_index=free_slot.index,
            live=True,
        )

    if commits:
        reservations[existing.index] = existing.value.with_fields(live=False)

    if cancels:
        reservations[existing.index] = existing.value.with_fields(live=False)
        slots[existing.value.slot_index] = TileSlot(
            owner=0,
            tile_version=0,
            allocation_generation=request.allocation_generation,
            allocated=False,
        )

    if frees:
        slots[target.index] = TileSlot(
            owner=0,
            tile_version=0,
            allocation_generation=request.allocation_generation,
            allocated=False,
        )

    # Which slot the acknowledgement names. A repeated allocate reports the slot
    # it already holds, which is what makes the response idempotent too, not just
    # the state.
    reported = free_slot.index
    if existing.valid:
        reported = existing.value.slot_index
    if is_free:
        reported = target.index

    return request.with_fields(
        accepted=allocates or commits or cancels or frees,
        matched=existing.valid,
        exhausted=is_alloc and not existing.valid and not has_capacity,
        slot_index=reported,
    )


@ac.rule
def answer_capacity_query(slots, query) -> CapacityQuery:
    """Report whether a free slot exists. A count is not available."""
    free_slot = ac.find(
        slots,
        where=lambda entry: not entry.allocated,
        key=lambda entry: entry.tile_version,
    )
    return query.with_fields(has_free_slot=free_slot.valid)


@ac.system
def trn_fre_system(
    request: AllocRequest, query: CapacityQuery
) -> tuple[AllocRequest, AllocRequest, AllocRequest, AllocRequest, CapacityQuery]:
    """One allocator pool, instantiated per PE for Local and once for Shared.

    The completion stream is split by operation, so each acknowledgement leaves
    on the port matching its request. That split is a plain route rather than
    four rule outputs because a rule that searches a table cannot also produce
    multiple outputs.
    """
    slots: list[TileSlot] = [0] * TILE_SLOTS_PER_POOL
    reservations: list[Reservation] = [0] * RESERVATION_SLOTS

    completed = serve_allocation(slots, reservations, request)
    answered = answer_capacity_query(slots, query)

    alloc_ack, commit_ack, cancel_ack, free_ack = completed.route(
        outputs=4,
        key=lambda completion: completion.operation & OPERATION_BRANCH_MASK,
        depth=1,
        latency=1,
    )
    return alloc_ack, commit_ack, cancel_ack, free_ack, answered
