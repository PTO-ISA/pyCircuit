# ndf: node=ndf://davincioo/DAV-TMU-TRF-REF-0001
"""Physical-version lease ledger for one PE's tile register file.

REF is the sole record of how many physical references a TileVersion has. It is a
ledger and not a lifetime authority: reaching zero references makes a version a
*reclaim candidate*, and actually freeing the logical version still needs
permission from RAT/FRE coordination. REF never frees anything itself.

The three mutating operations share one stream. A multi-input rule fires only
when every input has a token (Decision 0167/0168), and acquire, transfer and
release arrive independently, so three separate input ports would deadlock waiting
for each other. One stream is also the honest physical model, since a single-port
ledger serves one operation per cycle. A refcount query mutates nothing, so it
rides its own stream and is answered by a read-only rule over the same table.

One row is one lease of one TileVersion by one owner, and `count` is that lease's
reference count. A per-version query therefore reads a row instead of reducing
over the table, which matters because no reduction operator exists; see ref.md
for the limit this leaves.

A reclaim candidate travels on the acknowledgement record and is split out by a
route, because one rule cannot both search the table and produce several outputs.
That also states the timing correctly: zero references is an outcome of the
release that caused it, seen in the same tick.

Storage is a persistent indexed variable, which Decision 0151 classifies as
provisional state, so PYC and RTL must reject this leaf at an explicit boundary
and gfsim is the only backend that can execute it. ref.md records that boundary,
as bank.md does for the same reason.
"""

from __future__ import annotations

import agentic_circuit as ac

from designs.davincioo.contracts.tmu_trf import (
    ACK_BRANCH,
    LEASE_ACQUIRE,
    LEASE_COUNT_MAX,
    LEASE_RELEASE,
    LEASE_SLOTS,
    LEASE_TRANSFER,
    OUTCOME_BRANCH_MASK,
    RECLAIM_BRANCH,
    Lease,
    LeaseRequest,
    RefcountQuery,
)


@ac.rule
def serve_lease_request(leases, request) -> LeaseRequest:
    """Apply one acquire, transfer or release to the ledger, atomically.

    Two searches locate the work. `held` is the row this request already owns, and
    `free` is a row available for a new lease. A tombstoned row is not free: it
    still has late responses to drain, so reusing it would let a drained response
    land on a different lease.

    Acquire either increments the row it already has or claims a free one, so one
    owner holding a version twice is one row with a count of two rather than two
    rows. That is also what keeps the `held` search unambiguous: a free row is
    claimed only when `held` misses, so at most one live row ever matches the four
    identifying fields, and the `key` on that search is a required argument rather
    than a tie-break the design leans on. Saturation refuses the increment instead
    of wrapping, because a wrapped count would read as zero and make a live version
    look reclaimable.

    Transfer rewrites `owner` in place. That is what makes it have no unowned
    interval: there is no window in which the row is written but unowned, because
    the row never stops existing. Moving the lease to a different row would create
    exactly the interval the acceptance forbids.

    Release is idempotent because a row that is no longer live cannot be released
    again: `held` requires `live`, so a duplicate release finds nothing and
    changes nothing. The count reaching zero clears `live` and marks
    `release_pending`, and only then is a reclaim candidate emitted.

    The acknowledgement is always produced, so a request is never silently
    dropped -- including when the ledger is full, which is reported as
    `accepted=False`. `outcome` marks the release that reached zero, and the system
    splits that branch onto its own output.

    No response carries the physical slot. A lease is named by its owner, version
    and kind, so the slot is private layout, exactly as BANK exposes no sub-cell
    coordinate.
    """

    held = ac.find(
        leases,
        where=lambda entry: (
            entry.live
            and entry.owner == request.owner
            and entry.tile_version == request.tile_version
            and entry.lease_kind == request.lease_kind
        ),
        key=lambda entry: entry.count,
    )
    free = ac.find(
        leases,
        where=lambda entry: not entry.live and not entry.tombstone,
        key=lambda entry: entry.acquired_epoch,
    )

    is_acquire = request.operation == LEASE_ACQUIRE
    is_transfer = request.operation == LEASE_TRANSFER
    is_release = request.operation == LEASE_RELEASE

    saturated = held.value.count == LEASE_COUNT_MAX
    last_reference = held.value.count == 1

    increments = is_acquire and held.valid and not saturated
    inserts = is_acquire and not held.valid and free.valid
    transfers = is_transfer and held.valid
    releases = is_release and held.valid

    if increments:
        leases[held.index] = held.value.with_fields(
            count=held.value.count + 1,
            acquired_epoch=request.epoch,
        )

    if inserts:
        leases[free.index] = Lease(
            owner=request.owner,
            tile_version=request.tile_version,
            lease_kind=request.lease_kind,
            count=1,
            acquired_epoch=request.epoch,
            release_pending=False,
            tombstone=False,
            live=True,
        )

    if transfers:
        leases[held.index] = held.value.with_fields(owner=request.new_owner)

    if releases:
        leases[held.index] = held.value.with_fields(
            count=held.value.count - 1,
            live=not last_reference,
            release_pending=last_reference,
        )

    outcome = ACK_BRANCH
    if releases and last_reference:
        outcome = RECLAIM_BRANCH

    return request.with_fields(
        accepted=increments or inserts or transfers or releases,
        matched=held.valid,
        outcome=outcome,
        count=held.value.count,
    )


@ac.rule
def answer_refcount_query(leases, query) -> RefcountQuery:
    """Report the physical references a TileVersion has, without mutating.

    This rule only reads, so it may share the ledger with the mutating rule above
    while that rule remains its single writer.

    The query is qualified by version alone, not by owner, so it reads the row
    holding the most references for that version. When several owners hold the
    same version, that is one row's count rather than their sum, because no
    reduction operator exists; ref.md records the limit.
    """

    held = ac.find(
        leases,
        where=lambda entry: entry.live and entry.tile_version == query.tile_version,
        key=lambda entry: entry.count,
    )
    observed = held.value.count
    return query.with_fields(found=held.valid, count=observed)


@ac.system
def trf_ref_system(
    request: LeaseRequest, query: RefcountQuery
) -> tuple[LeaseRequest, LeaseRequest, RefcountQuery]:
    """One ledger per PE, serving one mutation and one query per cycle.

    The two rules are separate so a query never waits behind a mutation and never
    needs the mutating stream to have a token. The ledger has exactly one writer,
    which Decision 0151 requires.

    The acknowledgement stream is routed on `outcome`, so a caller waiting for
    acknowledgements and a reclaim consumer are independently backpressured. Both
    carry `LeaseRequest`, since a route cannot change a payload type.
    """

    leases: list[Lease] = [0] * LEASE_SLOTS

    completed = serve_lease_request(leases, request)
    answered = answer_refcount_query(leases, query)

    acknowledged, reclaim = completed.route(
        outputs=2,
        key=lambda completion: completion.outcome & OUTCOME_BRANCH_MASK,
        depth=1,
        latency=1,
    )

    return acknowledged, reclaim, answered
