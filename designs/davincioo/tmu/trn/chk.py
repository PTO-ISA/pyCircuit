# ndf: node=ndf://davincioo/DAV-TMU-TRN-CHK-0001
"""Checkpoints over one rename scope's speculative history.

CHK is the sole owner of captured recovery points. A checkpoint is one number --
how deep RAT's history stack was at capture -- plus the identity that names it.
It is not a copy of the map: RAT already retains every displaced mapping, so
copying them here would make two owners of one fact and would bound recovery by
CHK's capacity rather than by RAT's history.

CHK never writes RAT, FRE or STS. An unwind is answered with the frontier to
recover to and the number of rollbacks that reach it; the rollbacks themselves
are RAT transactions, and RAT re-checks each one against its own stack top. So a
recovery is qualified twice, by different owners, and neither trusts the other.

Staleness is derived, not stored. A checkpoint captured after the point being
recovered to has a *higher* frontier than the map will have once recovery
finishes, so it fails the ``current_frontier >= frontier`` test on any later
attempt to use it. Nothing has to sweep the table, which matters because a
search writes one row and there is no masked bulk update on this storage.

One rule serves all three operations, which makes the table single-port and keeps
its single writer. See designs/davincioo/contracts/tmu_trn.py for why a frontier
travels in the request instead of being read from RAT, and chk.md for why the
step sequencing is not owned here.
"""

from __future__ import annotations

import agentic_circuit as ac

from designs.davincioo.contracts.tmu_trn import (
    CHECKPOINT_SLOTS,
    CHK_BRANCH_MASK,
    CHK_CAPTURE,
    CHK_RELEASE,
    CHK_UNWIND,
    Checkpoint,
    CheckpointRequest,
)


@ac.rule
def serve_checkpoint_request(checkpoints, request):
    """Capture, resolve or release one checkpoint, atomically.

    Two searches locate the work. ``existing`` is the live record this request
    names, which makes a repeated capture return the record it already has
    instead of consuming a second row. ``free`` is a row available for a new
    capture.

    A capture stores the frontier the requester observed. An unwind reads it back
    and reports the distance to it. A release retires the record once its branch
    has resolved, which is what keeps a bounded pool from filling up in a program
    that never mispredicts.

    An unwind is refused as ``stale`` when the recorded frontier is deeper than
    the map's current one. That single comparison is the whole staleness rule: a
    checkpoint taken after the recovery point is exactly the case where its
    frontier is the deeper of the two, so it can never be used to restore a
    mapping newer than itself.

    The acknowledgement is always produced, so no request is dropped -- including
    a capture that arrives with the pool full, which is reported as ``exhausted``
    rather than stalled. chk.md records that gap.
    """

    existing = ac.find(
        checkpoints,
        where=lambda entry: (
            entry.live
            and entry.flow_key == request.flow_key
            and entry.checkpoint_id == request.checkpoint_id
        ),
        key=lambda entry: entry.frontier,
    )
    free = ac.find(
        checkpoints,
        where=lambda entry: not entry.live,
        key=lambda entry: entry.frontier,
    )

    is_capture = request.operation == CHK_CAPTURE
    is_unwind = request.operation == CHK_UNWIND
    is_release = request.operation == CHK_RELEASE

    # A checkpoint deeper than the map's current frontier describes a future that
    # no longer happened.
    reachable = request.current_frontier >= existing.value.frontier

    captures = is_capture and not existing.valid and free.valid
    unwinds = is_unwind and existing.valid and reachable
    releases = is_release and existing.valid

    if captures:
        checkpoints[free.index] = Checkpoint(
            flow_key=request.flow_key,
            checkpoint_id=request.checkpoint_id,
            frontier=request.current_frontier,
            live=True,
        )

    if releases:
        checkpoints[existing.index] = existing.value.with_fields(live=False)

    # An unwind leaves the record live. Recovery is driven by the caller against
    # RAT and can be re-attempted after backpressure, and a record retired here
    # would make the second attempt look like an unknown checkpoint.

    # How far back recovery goes, and how many rollbacks reach it. A refused
    # unwind reports the requester's own frontier and zero steps, so a caller that
    # ignores `accepted` recovers nothing rather than recovering to row zero.
    target = request.current_frontier
    distance = 0
    if unwinds:
        target = existing.value.frontier
        distance = request.current_frontier - existing.value.frontier

    return request.with_fields(
        accepted=captures or unwinds or releases,
        matched=existing.valid,
        exhausted=is_capture and not existing.valid and not free.valid,
        stale=is_unwind and existing.valid and not reachable,
        target_frontier=target,
        steps=distance,
    )


@ac.system
def trn_chk_system(
    request: CheckpointRequest,
) -> tuple[CheckpointRequest, CheckpointRequest, CheckpointRequest, CheckpointRequest]:
    """One checkpoint pool per rename scope, serving one operation per cycle.

    The pool is declared inside the system, so per-scope isolation is structural:
    a pool reached through a port could be shared by two instances.

    The acknowledgement stream is split by operation, so each answer leaves on the
    port matching its request. The fourth port carries an operation outside the
    closed set: it is refused rather than routed out of range, because a selector
    outside ``[0, outputs)`` is a deterministic runtime failure and answering a
    malformed request is cheaper than failing the model.
    """

    checkpoints: list[Checkpoint] = [0] * CHECKPOINT_SLOTS

    completed = serve_checkpoint_request(checkpoints, request)

    capture_ack, unwind_ack, release_ack, refused = completed.route(
        outputs=4,
        key=lambda completion: completion.operation & CHK_BRANCH_MASK,
        depth=1,
        latency=1,
    )
    return capture_ack, unwind_ack, release_ack, refused
