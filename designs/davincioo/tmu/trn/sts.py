# ndf: node=ndf://davincioo/DAV-TMU-TRN-STS-0001
"""Descriptor, coverage and publication status for one scope's tile versions.

STS is the sole owner of what a TileVersion *is*: its descriptor, how much of it
has been written, the first fault seen before publication, and whether it is
architectural. It holds no payload bytes, no physical slot and no logical name:
those belong to BANK, FRE and RAT.

A row is located by searching on ``(owner, tile_version)`` rather than by
indexing, because version identifiers are sparse: only a bounded number of
versions are live at once out of a 16-bit space. That is the opposite of RAT's
map, where a logical name is a dense index.

The four mutating operations share one stream. A multi-input rule fires only when
every input has a token (Decision 0167/0168), and a reserve, a write, a publish
and a rollback arrive independently, so four input ports would deadlock waiting
for each other. A status query mutates nothing, so it rides its own stream and is
answered by a read-only rule over the same table.

Publication is the one operation that validates before it changes anything. An
incompatible descriptor is refused with ``rejected`` and leaves every field as it
was, which is why compatibility is computed from the committed row rather than
assumed by the caller.

See designs/davincioo/contracts/tmu_trn.py for why allocation and initialization
are two masks, why the fault code retains the first fault, and why
``contents_defined`` is computed instead of stored.
"""

from __future__ import annotations

import agentic_circuit as ac

from designs.davincioo.contracts.tmu_trn import (
    FAULT_NONE,
    STATUS_ROWS,
    STS_BRANCH_MASK,
    STS_PUBLISH,
    STS_RESERVE,
    STS_ROLLBACK,
    STS_WRITE,
    StatusQuery,
    StatusRequest,
    StatusRow,
)


@ac.rule
def serve_status_request(rows, request):
    """Apply one reserve, write, publish or rollback to the status table.

    Two searches locate the work. ``existing`` is the live row this request
    names; ``free`` is a row available for a new version. A reserve claims a free
    row only when ``existing`` misses, so one version has at most one row and the
    ``existing`` search stays unambiguous.

    A write merges coverage rather than replacing it, because writes to one
    version arrive as several partial updates and a replacing update would erase
    the coverage of the ones before it. The fault code keeps the *first* fault
    instead of the latest, so the fault a caller reads is the one that caused the
    others.

    A publish validates the descriptor against the committed row before any
    visible state changes: an incompatible publication is refused, not applied
    and then corrected. That ordering is the acceptance, so it is expressed as a
    condition on the write rather than as a later check.

    A rollback deletes only a speculative row of the same allocation generation. A
    published row and a newer generation are both unreachable to it, which is what
    keeps a stale recovery from erasing architectural state.

    The acknowledgement is always produced, so no request is dropped -- including
    a reserve that arrives with the table full, which is reported as ``exhausted``
    rather than stalled. sts.md records that gap.
    """

    existing = ac.find(
        rows,
        where=lambda entry: (
            entry.live
            and entry.owner == request.owner
            and entry.tile_version == request.tile_version
        ),
        key=lambda entry: entry.allocation_generation,
    )
    free = ac.find(
        rows,
        where=lambda entry: not entry.live,
        key=lambda entry: entry.tile_version,
    )

    is_reserve = request.operation == STS_RESERVE
    is_write = request.operation == STS_WRITE
    is_publish = request.operation == STS_PUBLISH
    is_rollback = request.operation == STS_ROLLBACK

    same_generation = (
        existing.value.allocation_generation == request.allocation_generation
    )
    # A published row is architectural, so nothing speculative may still touch it.
    mutable = existing.valid and same_generation and not existing.value.published

    # Compatibility is checked against what the row already holds, so a publish
    # cannot smuggle in a different descriptor while making the version visible.
    compatible = (
        existing.value.dtype == request.dtype
        and existing.value.layout == request.layout
        and existing.value.shape == request.shape
    )

    reserves = is_reserve and not existing.valid and free.valid
    writes = is_write and mutable
    publishes = is_publish and existing.valid and same_generation and compatible
    rolls_back = is_rollback and mutable and existing.value.speculative

    # Coverage merges; it never replaces. Both masks are computed once and used
    # for the row and the acknowledgement, so the answer cannot describe a
    # different row than the one written.
    merged_allocation = existing.value.allocation_mask | request.allocation_mask
    merged_initialized = existing.value.initialized_mask | request.initialized_mask

    # The first fault is retained. A row with no fault yet takes the incoming one.
    retained_fault = existing.value.fault_code
    if existing.value.fault_code == FAULT_NONE:
        retained_fault = request.fault_code

    # Every operation writes one row, so they are expressed as one write of a
    # selected index and a selected value. Two writes of one owner must be
    # provably disjoint or provably exclusive, and "a reserve claims a free row
    # while the others rewrite the located one" is true but not derivable from
    # the guards, so the exclusion is made structural: there is only one write.
    created = StatusRow(
        owner=request.owner,
        tile_version=request.tile_version,
        allocation_generation=request.allocation_generation,
        dtype=request.dtype,
        layout=request.layout,
        shape=request.shape,
        valid_region=request.valid_region,
        allocation_mask=request.allocation_mask,
        # A reserved version has been allocated, never written. This zero is
        # the acceptance "allocation does not imply payload definedness",
        # expressed as state rather than as a comment.
        initialized_mask=0,
        fault_code=FAULT_NONE,
        speculative=True,
        published=False,
        live=True,
    )
    updated = existing.value.with_fields(
        dtype=request.dtype,
        layout=request.layout,
        shape=request.shape,
        valid_region=request.valid_region,
        allocation_mask=merged_allocation,
        initialized_mask=merged_initialized,
        fault_code=retained_fault,
    )
    promoted = existing.value.with_fields(
        speculative=False,
        published=True,
    )
    retired = existing.value.with_fields(
        speculative=False,
        live=False,
    )

    target = existing.index
    if reserves:
        target = free.index

    written = created
    if writes:
        written = updated
    if publishes:
        written = promoted
    if rolls_back:
        written = retired

    if reserves or writes or publishes or rolls_back:
        rows[target] = written

    # What the acknowledgement reports about coverage. A reserve reports the
    # allocation it just made with nothing initialized; every other operation
    # reports the row's coverage as this operation leaves it.
    reported_allocation = merged_allocation
    reported_initialized = merged_initialized
    if reserves:
        reported_allocation = request.allocation_mask
        reported_initialized = 0

    return request.with_fields(
        accepted=reserves or writes or publishes or rolls_back,
        matched=existing.valid,
        # A refused publication is a distinct outcome from a missing row, so a
        # caller reading only `accepted` cannot mistake one for the other.
        rejected=is_publish and existing.valid and same_generation and not compatible,
        exhausted=is_reserve and not existing.valid and not free.valid,
        published=existing.value.published,
        allocation_mask=reported_allocation,
        initialized_mask=reported_initialized,
        fault_code=retained_fault,
    )


@ac.rule
def answer_status_query(rows, query):
    """Report one version's status snapshot without mutating it.

    This rule only reads, so it may share the table with the mutating rule above
    while that rule remains its single writer.

    ``contents_defined`` compares the two masks here rather than reading a stored
    bit, so a version whose allocation grew after its last write reads as
    incompletely defined without anything having to remember to clear a flag.
    """

    row = ac.find(
        rows,
        where=lambda entry: (
            entry.live
            and entry.owner == query.owner
            and entry.tile_version == query.tile_version
        ),
        key=lambda entry: entry.allocation_generation,
    )
    return query.with_fields(
        dtype=row.value.dtype,
        layout=row.value.layout,
        shape=row.value.shape,
        valid_region=row.value.valid_region,
        allocation_mask=row.value.allocation_mask,
        initialized_mask=row.value.initialized_mask,
        fault_code=row.value.fault_code,
        found=row.valid,
        speculative=row.value.speculative,
        published=row.value.published,
        contents_defined=row.valid
        and row.value.initialized_mask == row.value.allocation_mask,
    )


@ac.system
def trn_sts_system(
    request: StatusRequest, query: StatusQuery
) -> tuple[StatusRequest, StatusRequest, StatusRequest, StatusRequest, StatusQuery]:
    """One status table per rename scope, serving one mutation per cycle.

    The two rules are separate so a query never waits behind a mutation and never
    needs the mutating stream to have a token. The table has exactly one writer,
    which Decision 0151 requires.

    The acknowledgement stream is split by operation, so each answer leaves on the
    port matching its request and a blocked reserve consumer cannot stall a
    publish. That split is a plain route rather than four rule outputs because a
    rule that searches a table cannot also produce multiple outputs.
    """

    rows: list[StatusRow] = [0] * STATUS_ROWS

    completed = serve_status_request(rows, request)
    answered = answer_status_query(rows, query)

    reserve_ack, write_ack, publish_ack, rollback_ack = completed.route(
        outputs=4,
        key=lambda completion: completion.operation & STS_BRANCH_MASK,
        depth=1,
        latency=1,
    )
    return reserve_ack, write_ack, publish_ack, rollback_ack, answered
