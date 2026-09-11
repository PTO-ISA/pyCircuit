"""Shared contracts for one PE's tile register file.

Two independent concerns live here because both belong to TRF.  The first is
BANK's physical byte access; the second, further down, is REF's physical-version
lease ledger.  They share no state and no record: BANK stores bytes and knows
nothing about leases, REF counts references and never touches a payload.

BANK owns raw payload bytes and nothing else.  ``BankAccess`` is therefore a
physical record: it carries the request identity needed to route a response and
to match a cancellation, plus the single physical coordinate ``row``, and it
carries no shape, dtype, layout, valid-region or definedness field.

Access granularity is one whole 128-byte cell.  Clients address cells, never
parts of cells, so ``row`` is the only coordinate and no interface can reach a
fragment of a cell.  ``word0`` .. ``word15`` are not a coordinate: they are the
sixteen pieces one indivisible payload is cut into because ``ac.bits`` admits
widths in ``[1, 64]`` (Decision 0032), and every path reads or writes all
sixteen together.  Nothing selects among them.

One record covers reads, writes and invalidations and accumulates results in
place, the same pattern ``BankGrant`` uses in designs/davincioo/tmu/bgf/arb.py.
ARB already merges reads and writes into one granted stream per bank, so a
single-port bank sees one access at a time by construction.  On the way out the
same sixteen fields carry the observed cell, so a write's fields are the data to
store and a read's fields are the data returned.

``CellData`` is the stored element and deliberately excludes request identity.
Storing a request's identity in a cell would make a later read return the
*previous writer's* identity instead of the current requester's, which would
misroute the response.
"""

from __future__ import annotations

import agentic_circuit as ac

# The constants below are the CELL schema.  designs/davincioo/tmu/trf/cell.md is
# a state_schema and authorises no independent leaf, so its geometry lives in this
# contract and its state lives in the two arrays bank.py elaborates.  A cell.py
# module would create a second owner of the same bytes.

# Rows of one PE-private bank.  designs/davincioo/tmu/trf/bank.md freezes 256
# rows; one row holds one 128-byte cell, so one row is one entry.
ROWS_PER_BANK = 256
ROW_INDEX_MASK = ROWS_PER_BANK - 1

# One cell is 128 bytes.  It is stored as WORDS_PER_CELL scalar fields only
# because 64 bits is the widest integer the frontend admits; the split is a
# representation detail with no addressable meaning.
CELL_BYTES = 128
WORDS_PER_CELL = 16
assert WORDS_PER_CELL * 64 == CELL_BYTES * 8

# Read latency in cycles as seen at BANK's output, not the latency of one
# internal stage.  The rule that touches the arrays contributes
# RULE_ACCESS_LATENCY, so bank.py delays its result by the remainder to make the
# total exactly CELL_READ_LATENCY.
CELL_READ_LATENCY = 2
RULE_ACCESS_LATENCY = 1
CELL_OUTPUT_STAGES = CELL_READ_LATENCY - RULE_ACCESS_LATENCY
assert CELL_OUTPUT_STAGES >= 1

# Closed access kinds.  A read observes, a write replaces a whole cell, and an
# invalidation installs a new generation without touching payload bytes.
ACCESS_READ = 0
ACCESS_WRITE = 1
ACCESS_INVALIDATE = 2


@ac.struct
class CellData:
    word0: ac.u64
    word1: ac.u64
    word2: ac.u64
    word3: ac.u64
    word4: ac.u64
    word5: ac.u64
    word6: ac.u64
    word7: ac.u64
    word8: ac.u64
    word9: ac.u64
    word10: ac.u64
    word11: ac.u64
    word12: ac.u64
    word13: ac.u64
    word14: ac.u64
    word15: ac.u64


@ac.struct
class BankAccess:
    sequence: ac.u16
    source_class: ac.u8
    requester: ac.u8
    flow_id: ac.u16
    launch_generation: ac.u16
    thread_id: ac.u16
    block_id: ac.u16
    operation_id: ac.u16
    tile_id: ac.u16
    tile_version: ac.u16
    allocation_generation: ac.u16
    request_id: ac.u16
    cell_key: ac.u16
    row: ac.u16
    response_route: ac.u8
    kind: ac.u8
    installed_generation: ac.u16
    stored_generation: ac.u16
    current: bool
    applied: bool
    word0: ac.u64
    word1: ac.u64
    word2: ac.u64
    word3: ac.u64
    word4: ac.u64
    word5: ac.u64
    word6: ac.u64
    word7: ac.u64
    word8: ac.u64
    word9: ac.u64
    word10: ac.u64
    word11: ac.u64
    word12: ac.u64
    word13: ac.u64
    word14: ac.u64
    word15: ac.u64


# ---------------------------------------------------------------------------
# REF: physical-version lease ledger
# ---------------------------------------------------------------------------
#
# REF counts physical references to a TileVersion.  It is a ledger, not a
# lifetime authority: reaching zero references makes a version a reclaim
# candidate, and freeing the logical version still requires permission from
# RAT/FRE coordination.
#
# The three mutating operations share one record and one stream.  That is forced
# rather than chosen: a multi-input rule fires only when *every* input has a token
# (Decision 0167/0168), and acquire, transfer and release arrive independently, so
# four separate input ports would deadlock waiting for each other.  One stream
# also matches the hardware, since a single-port table serves one operation per
# cycle.  A refcount query is different: it mutates nothing, so it rides its own
# stream and is answered by a read-only rule over the same table.

# A lease is named by (owner, tile_version, lease_kind), never by its physical
# slot: the slot is REF's private layout and no response carries it, for the same
# reason BankAccess exposes no sub-cell coordinate.
#
# Lease slots in one ledger.  designs/davincioo/tmu/trf/ref.md lists table
# capacity as an open decision; this freezes it, and the free-slot search makes
# the value a parameter rather than an assumption baked into the logic.
LEASE_SLOTS = 64

# Lease kinds.  A read lease and a write lease are counted separately because a
# reclaim candidate must not appear while a writer still holds the version.
LEASE_KIND_READ = 0
LEASE_KIND_WRITE = 1

# Closed mutating operations, carried in one stream.
LEASE_ACQUIRE = 0
LEASE_TRANSFER = 1
LEASE_RELEASE = 2

# Reference count width.  A ledger row saturates rather than wrapping, so an
# overflowed acquire is refused instead of aliasing a live version onto zero.
LEASE_COUNT_MAX = 0xFFFF


@ac.struct
class Lease:
    owner: ac.u16
    tile_version: ac.u16
    lease_kind: ac.u8
    count: ac.u16
    acquired_epoch: ac.u16
    release_pending: bool
    tombstone: bool
    live: bool


@ac.struct
class LeaseRequest:
    operation: ac.u8
    requester: ac.u8
    request_id: ac.u16
    response_route: ac.u8
    owner: ac.u16
    new_owner: ac.u16
    tile_version: ac.u16
    lease_kind: ac.u8
    epoch: ac.u16
    accepted: bool
    matched: bool
    outcome: ac.u8
    count: ac.u16


# Route branches carried in ``outcome``.  A release that reached zero references
# takes RECLAIM_BRANCH so it reaches the reclaim consumer; every other outcome
# takes ACK_BRANCH.  This is an integer rather than a bool because a route key
# must lower to an integer (``ACPY-QUEUE-006``).
ACK_BRANCH = 0
RECLAIM_BRANCH = 1
OUTCOME_BRANCH_MASK = 1


@ac.struct
class RefcountQuery:
    request_id: ac.u16
    response_route: ac.u8
    tile_version: ac.u16
    count: ac.u16
    found: bool
