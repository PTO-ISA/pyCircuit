# ndf: node=ndf://davincioo/DAV-TMU-TRN-RAT-0001
"""Logical tile name to physical version, for one rename scope.

RAT is the sole owner of the name-to-version mapping. It says which version a
logical tile name currently means and which version of that name is
architectural. It holds no payload bytes, no descriptor and no physical slot:
those belong to BANK, STS and FRE.

Two kinds of storage sit here, and they are shaped differently on purpose. The
map is indexed by logical name, because a name is an index: the namespace is
dense and every name exists from reset. The history is a stack, because rollback
must proceed youngest-first and ``ac.find``'s key selects the minimum key, so age
cannot be expressed as a search at all. Making the history a stack turns
youngest-first into the shape of the storage rather than a property of a
selection.

One rule serves all three mutating operations, which makes the map single-port: a
rule admits one token per tick. A second read-only rule answers rename lookups
over the same map without ever writing it.

The Local map is instantiated per PE, so two PEs using the same logical name
cannot collide -- that isolation is structural, not a checked invariant. The
Shared map is the same system instantiated once per core, because a core-owned
map written by four per-PE rules would give one table four write endpoints.

See designs/davincioo/contracts/tmu_trn.py for why the operations share one
stream, why LRM contributes no type of its own, and why a reset row reads as
"no version yet" instead of as version zero.
"""

from __future__ import annotations

import agentic_circuit as ac

from designs.davincioo.contracts.tmu_trn import (
    HISTORY_INDEX_MASK,
    LOGICAL_TILE_REGS,
    MAP_HISTORY_DEPTH,
    RAT_BRANCH_MASK,
    RAT_PUBLISH,
    RAT_ROLLBACK,
    RAT_SWAP,
    MapHistory,
    MapRequest,
    MapRow,
    RenameLookup,
)


@ac.rule
def serve_map_request(history_depth, map_rows, history, request):
    """Apply one swap, publish or rollback to the map, atomically.

    Two reads locate the work, and neither is a search. ``row`` is the addressed
    name's current mapping, reached by index. ``newest`` is the top of the
    history stack, which is the only entry a rollback may undo.

    A swap installs the new version and pushes the row it displaced. Both writes
    are guarded by the same condition, so a mapping is never overwritten without
    the record that can restore it -- that pairing is what makes the swap
    all-or-none, and losing it would make a rollback silently restore the wrong
    version rather than fail.

    A publish makes the current version architectural. It is qualified by
    ``map_generation`` so a publish that lost its race against a newer swap does
    not promote the newer version by accident.

    A rollback is generation-qualified against the stack top, so a stale
    checkpoint cannot restore a mapping newer than the one it captured, and the
    stack order gives youngest-first without any age comparison. The complete old
    row is restored, including whether the name was valid at all, because a name
    that had no version before the swap must go back to having none.

    The acknowledgement is always produced, so no request is dropped -- including
    a swap that arrives with the history full, which is reported as
    ``history_full`` rather than stalled. rat.md records that gap.
    """

    logical = request.logical_id
    row = map_rows[logical]

    top = (history_depth - 1) & HISTORY_INDEX_MASK
    newest = history[top]

    is_swap = request.operation == RAT_SWAP
    is_publish = request.operation == RAT_PUBLISH
    is_rollback = request.operation == RAT_ROLLBACK

    history_full = history_depth == MAP_HISTORY_DEPTH
    has_history = history_depth != 0

    swaps = is_swap and not history_full
    publishes = (
        is_publish and row.valid and row.map_generation == request.map_generation
    )
    rolls_back = (
        is_rollback and has_history and newest.map_generation == request.map_generation
    )

    # All three operations write one map row, so they are expressed as one
    # write of a selected index and a selected value rather than as three
    # writes. Two writes of the same owner must be provably disjoint or
    # provably exclusive, and "a swap and a rollback address different names"
    # is true but not derivable from the guards, so the exclusion is made
    # structural: there is only one write.
    installed = MapRow(
        current_version=request.tile_version,
        map_generation=request.map_generation,
        published_version=row.published_version,
        valid=True,
        published=False,
    )
    promoted = row.with_fields(
        published_version=row.current_version,
        published=True,
    )
    restored = MapRow(
        current_version=newest.old_version,
        map_generation=newest.old_generation,
        published_version=newest.old_published_version,
        valid=newest.old_valid,
        published=newest.old_published,
    )

    target = logical
    if rolls_back:
        target = newest.logical_id

    written = installed
    if publishes:
        written = promoted
    if rolls_back:
        written = restored

    if swaps or publishes or rolls_back:
        map_rows[target] = written

    if swaps:
        history[history_depth & HISTORY_INDEX_MASK] = MapHistory(
            logical_id=logical,
            old_version=row.current_version,
            old_generation=row.map_generation,
            old_published_version=row.published_version,
            map_generation=request.map_generation,
            old_valid=row.valid,
            old_published=row.published,
        )

    # The stack pointer moves once, from one value computed on all paths, and
    # only on the operations that actually push or pop. A swap that found the
    # history full does not move it, which is what lets the refused swap be
    # retried without first repairing state.
    #
    # The write is guarded rather than unconditional because a state proposal
    # must carry the presence of the path that makes it: a rule that mixes an
    # unconditional scalar write with conditional array writes has no presence
    # to attach to the scalar one and is rejected as
    # `state proposal presence must imply the rule condition`.
    next_depth = history_depth
    if swaps:
        next_depth = history_depth + 1
    if rolls_back:
        next_depth = history_depth - 1
    if swaps or rolls_back:
        history_depth = next_depth

    # Which version the acknowledgement names: the one displaced by a swap, the
    # one a publish made architectural, or the one a rollback restored. A caller
    # therefore learns the version it needs without a second lookup.
    reported = row.current_version
    if rolls_back:
        reported = newest.old_version

    # A rollback names no logical id -- the stack decides which name it undoes --
    # so the acknowledgement reports the name that was restored. Without this the
    # caller would have to guess, and the request's own field carries nothing.
    reported_logical = request.logical_id
    if rolls_back:
        reported_logical = newest.logical_id

    # ``matched`` means "the thing this operation names exists". For a swap or a
    # publish that is the addressed row; for a rollback it is the stack top, and
    # reporting the addressed row instead would describe an unrelated name.
    matched = row.valid
    if is_rollback:
        matched = rolls_back

    return request.with_fields(
        accepted=swaps or publishes or rolls_back,
        matched=matched,
        history_full=is_swap and history_full,
        logical_id=reported_logical,
        reported_version=reported,
        history_depth=next_depth,
    )


@ac.rule
def answer_rename_lookup(map_rows, lookup):
    """Report a logical name's current mapping without mutating it.

    This rule only reads, so it may share the map with the mutating rule above
    while that rule remains its single writer. The answer carries both the
    current version and the last published one, so a consumer that must not read
    speculative state decides here instead of asking STS.
    """

    row = map_rows[lookup.logical_id]
    return lookup.with_fields(
        tile_version=row.current_version,
        map_generation=row.map_generation,
        published_version=row.published_version,
        found=row.valid,
        published=row.published,
    )


@ac.system
def trn_rat_system(
    request: MapRequest, lookup: RenameLookup
) -> tuple[MapRequest, MapRequest, MapRequest, MapRequest, RenameLookup]:
    """One rename map, instantiated per PE for Local and once for Shared.

    The two rules are separate so a lookup never waits behind a swap and never
    needs the mutating stream to have a token.

    The acknowledgement stream is split by operation, so each answer leaves on
    the port matching its request. The fourth port carries an operation outside
    the closed set: it is refused rather than routed out of range, because a
    selector outside ``[0, outputs)`` is a deterministic runtime failure and
    answering a malformed request is cheaper than failing the model. That split
    is a plain route rather than four rule outputs because a rule that indexes
    state cannot also produce multiple outputs.
    """

    history_depth: ac.u6 = 0
    map_rows: list[MapRow] = [0] * LOGICAL_TILE_REGS
    history: list[MapHistory] = [0] * MAP_HISTORY_DEPTH

    completed = serve_map_request(history_depth, map_rows, history, request)
    answered = answer_rename_lookup(map_rows, lookup)

    swap_ack, publish_ack, rollback_ack, refused = completed.route(
        outputs=4,
        key=lambda completion: completion.operation & RAT_BRANCH_MASK,
        depth=1,
        latency=1,
    )
    return swap_ack, publish_ack, rollback_ack, refused, answered
