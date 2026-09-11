"""Shared contracts for one core's tile rename and status owners.

One file holds FRE, RAT, STS and CHK because they describe one lifetime from
four sides, and a version identifier that drifted between them would be a
silent aliasing bug rather than a type error.  LRM has no types of its own;
see the alias note above ``MapRequest``.

FRE owns the free physical tile slots.  It is the allocator that ALC is not:
designs/davincioo/tmu/trf/alc.md records that ALC's whole proposed scope folds
into this interface, and nothing else in the design program reserves storage.

**The physical resource is a fixed set of fixed-size slots, not a heap.**  TRF is
a register *file*, so this is what the hardware is, and it is also the only shape
the frontend can express: allocating a variable-length extent needs a
contiguous-run search across table entries, and there is no reduction or
run-length primitive.  Selecting one free slot is a single ``ac.find``.

Slot geometry is derived from BANK rather than chosen here, so the two cannot
drift.  A pool covers exactly the cells one PE owns.

**The four mutating operations share one record and one stream.**  That is forced,
not preferred: a multi-input rule fires only when *every* input has a token
(Decision 0167/0168), and allocate, commit, cancel and free arrive independently,
so four input ports would deadlock waiting for each other.  One stream also
matches the hardware, since a single-port allocator serves one operation per
cycle.  A capacity query mutates nothing, so it rides its own stream and is
answered by a read-only rule over the same pool.

**A slot carries ``allocated``, not ``free``.**  Decision 0151 admits an all-zero
initial image only, so storing the allocated bit makes all-zero mean "every slot
is available".  That satisfies fre.md's nonzero-reset requirement without a reset
FSM.  The same constraint is why a slot cannot store its own index: responses
therefore carry ``ac.find``'s index, and ``slot_index`` is exactly as wide as the
pool needs.

**Local and Shared pools share one geometry, and Shared is a separate system.**
A slot index is exactly wide enough for its pool and the frontend has no width
conversion primitive, so two pools of different capacity could not share one
request record.  Shared also cannot sit inside the per-PE instance: it is
core-owned, and four per-PE rules writing one Shared pool would give that Table
four write endpoints, which Decision 0151 forbids.

``scope`` is carried for the parent, which uses it to pick the Local or the
Shared instance.  FRE never reads it, for the same reason BANK never reads
``cell_key``: whoever routes on a field must be the only one who interprets it.
"""

import agentic_circuit as ac

from designs.davincioo.contracts.tmu_bgf import BANK_PARTITIONS
from designs.davincioo.contracts.tmu_trf import ROWS_PER_BANK

# Physical tile slots in one pool.  A pool covers exactly one PE's cells, so the
# slot size follows from BANK's geometry instead of being asserted here.
#
# ``SLOT_INDEX_BITS`` is spelled as a literal in the ``ac.bits[6]`` annotations
# below because the frontend requires a static width there (``ACPY-TYPE-003``); it
# parses annotations as source text and cannot see a constant's value.  The
# assertion is what keeps the literal and the capacity from drifting apart.
TILE_SLOTS_PER_POOL = 64
SLOT_INDEX_BITS = 6
assert 1 << SLOT_INDEX_BITS == TILE_SLOTS_PER_POOL

CELLS_PER_PE = BANK_PARTITIONS * ROWS_PER_BANK
CELLS_PER_TILE_SLOT = CELLS_PER_PE // TILE_SLOTS_PER_POOL
assert CELLS_PER_TILE_SLOT * TILE_SLOTS_PER_POOL == CELLS_PER_PE

# In-flight cancellable reservations.  A row lives only until its transaction
# commits or cancels, so this bounds concurrent rename transactions rather than
# live tile versions.
RESERVATION_SLOTS = 32
RESERVATION_INDEX_BITS = 5
assert 1 << RESERVATION_INDEX_BITS == RESERVATION_SLOTS

# Closed mutating operations, carried in one stream.  These double as the route
# branch, because each acknowledgement returns to the port matching its
# operation; no separate outcome field is needed.
FRE_ALLOC = 0
FRE_COMMIT = 1
FRE_CANCEL = 2
FRE_FREE = 3
OPERATION_BRANCH_MASK = 3

# Allocation scopes.  Read by the parent to select an instance, never by FRE.
SCOPE_LOCAL = 0
SCOPE_SHARED = 1


# One physical tile slot.  Holds no payload bytes; those live in BANK.
@ac.struct
class TileSlot:
    owner: ac.u16
    tile_version: ac.u16
    allocation_generation: ac.u16
    allocated: bool


# One uncommitted, still-cancellable reservation, keyed by RenameTxnKey.  The row
# is what makes a repeated allocate idempotent.  It is cleared on both commit and
# cancel, so this table holds in-flight transactions only, and a committed version
# is recorded by its slot alone.
@ac.struct
class Reservation:
    txn_key: ac.u32
    owner: ac.u16
    slot_index: ac.bits[6]
    live: bool


# One allocate, commit, cancel or free, and its acknowledgement.
#
# ``logical_permission`` and ``refs_zero`` are the two preconditions fre.md
# requires before a version may be freed.  They are checked here rather than
# trusted: FRE cannot see REF's ledger or RAT's lifetime state, so the coordinator
# that can must present both, and FRE refuses a free that is missing either one.
@ac.struct
class AllocRequest:
    operation: ac.u8
    txn_key: ac.u32
    requester: ac.u8
    request_id: ac.u16
    response_route: ac.u8
    scope: ac.u8
    owner: ac.u16
    tile_version: ac.u16
    allocation_generation: ac.u16
    logical_permission: bool
    refs_zero: bool
    accepted: bool
    matched: bool
    exhausted: bool
    slot_index: ac.bits[6]


# Whether a pool still has a free slot.
#
# ``has_free_slot`` is a bit, not a count.  There is no reduction operator, so the
# number of free slots cannot be computed; the free-slot search answers only
# whether one exists.  designs/davincioo/tmu/trf/alc.md folds capacity projection
# into FRE, and this is how much of it FRE can actually answer.
@ac.struct
class CapacityQuery:
    request_id: ac.u16
    response_route: ac.u8
    scope: ac.u8
    has_free_slot: bool


# ---------------------------------------------------------------------------
# RAT — logical name to physical version, and the speculative history that can
# take a mapping back.
#
# The map is indexed by logical name directly, because a name *is* an index: the
# namespace is dense and every name exists from reset.  That is the opposite of
# FRE's pool, where a slot is found by searching, and it is why RAT needs no
# ``ac.find`` over its map at all.
#
# The history is a stack rather than a searched table.  Rollback must proceed
# youngest-first, and ``ac.find``'s key selects the *minimum* key, so age cannot
# be expressed as a search at all.  A stack makes youngest-first the shape of the
# storage instead of a property the selection has to be trusted to have.
#
# ``LOGICAL_INDEX_BITS`` is spelled as a literal in the ``ac.u6`` annotations
# below because the frontend requires a static width there; the assertion is what
# keeps the literal and the namespace size from drifting apart.  A name is
# therefore exactly as wide as the namespace, so a name outside it is
# unrepresentable rather than wrapped onto a live mapping.
LOGICAL_TILE_REGS = 64
LOGICAL_INDEX_BITS = 6
assert 1 << LOGICAL_INDEX_BITS == LOGICAL_TILE_REGS

# Uncommitted map entries one instance can hold.  This bounds speculation depth,
# not the number of live versions: an entry is popped by the rollback or dropped
# by the publish that resolves it.
MAP_HISTORY_DEPTH = 32
HISTORY_INDEX_BITS = 5
HISTORY_INDEX_MASK = MAP_HISTORY_DEPTH - 1
assert 1 << HISTORY_INDEX_BITS == MAP_HISTORY_DEPTH

# Closed mutating operations, carried in one stream for the same reason FRE's
# are: a multi-input rule fires only when every input has a token (Decision
# 0167/0168), and a swap, a publish and a rollback arrive independently.
RAT_SWAP = 0
RAT_PUBLISH = 1
RAT_ROLLBACK = 2
# An operation outside the closed set.  It is a real branch rather than a gap in
# the mask: the acknowledgement leaves on its own port refused, so a malformed
# request is answered instead of routed out of range.
RAT_UNKNOWN = 3
RAT_BRANCH_MASK = 3


# One logical name's mapping.  ``valid`` is false in the all-zero reset image, so
# a name reads as "no version yet" rather than as a mapping to version zero.
@ac.struct
class MapRow:
    current_version: ac.u16
    map_generation: ac.u16
    published_version: ac.u16
    valid: bool
    published: bool


# One displaced mapping, pushed by the swap that displaced it.
#
# The entry stores the complete old row rather than a handle to it, because a
# rollback must restore the exact prior mapping and there is nowhere else the
# prior value survives: the swap that displaced it overwrote the only copy.
@ac.struct
class MapHistory:
    logical_id: ac.u6
    old_version: ac.u16
    old_generation: ac.u16
    old_published_version: ac.u16
    map_generation: ac.u16
    old_valid: bool
    old_published: bool


# One swap, publish or rollback, and its acknowledgement.
#
# LRM is an alias, not a second map: designs/davincioo/tmu/trn/lrm.md records
# that the Local rename map is the Local projection of RAT.  A Local request is
# this record with ``scope = SCOPE_LOCAL``, answered by the per-PE instance.
# There is no LRM type and no LRM module; adding either would duplicate the
# mapping the disposition forbids duplicating.
@ac.struct
class MapRequest:
    operation: ac.u8
    txn_key: ac.u32
    requester: ac.u8
    request_id: ac.u16
    response_route: ac.u8
    scope: ac.u8
    logical_id: ac.u6
    tile_version: ac.u16
    map_generation: ac.u16
    reported_version: ac.u16
    accepted: bool
    matched: bool
    history_full: bool
    history_depth: ac.u6


# One lookup of a logical name's current mapping.
#
# Lookup rides its own stream and is answered by a read-only rule, so a rename
# reading its sources never waits behind a rollback.  ``published`` distinguishes
# an architectural version from a speculative one; a consumer that must not read
# speculative state checks it here rather than asking STS.
@ac.struct
class RenameLookup:
    requester: ac.u8
    request_id: ac.u16
    response_route: ac.u8
    scope: ac.u8
    logical_id: ac.u6
    tile_version: ac.u16
    map_generation: ac.u16
    published_version: ac.u16
    found: bool
    published: bool


# ---------------------------------------------------------------------------
# STS — what a version is, how much of it has been written, and whether it is
# architectural.  It holds no bytes: designs/davincioo/tmu/trf/bank.py owns those.
#
# A row is located by searching, not by indexing, because a version identifier is
# sparse where a logical name is dense.  That is the same distinction RAT's two
# storages make, applied across two modules.
STATUS_ROWS = 64

# Closed mutating operations, carried in one stream.  These double as the route
# branch: four operations exactly fill a two-bit branch, so every acknowledgement
# has a port and no selector can leave the declared range.
STS_RESERVE = 0
STS_WRITE = 1
STS_PUBLISH = 2
STS_ROLLBACK = 3
STS_BRANCH_MASK = 3

# No fault retained yet.  Zero is "no fault" so that the all-zero reset image and
# a freshly reserved row agree without a reset pass.
FAULT_NONE = 0


# One TileVersion's descriptor, coverage and publication status.
#
# ``allocation_mask`` and ``initialized_mask`` are separate because allocation
# does not imply definedness: reserving a version says where it may be written,
# never that anything was.  Collapsing them into one mask would make a reserved
# version read as fully defined, which is the exact confusion sts.md forbids.
#
# Both are 64-bit element-coverage masks.  That is the bounded representation the
# card leaves open, chosen because it is the widest value the payload family
# admits; a version needing finer coverage needs a different representation, not
# a wider field.
@ac.struct
class StatusRow:
    owner: ac.u16
    tile_version: ac.u16
    allocation_generation: ac.u16
    dtype: ac.u8
    layout: ac.u8
    shape: ac.u32
    valid_region: ac.u32
    allocation_mask: ac.u64
    initialized_mask: ac.u64
    fault_code: ac.u8
    speculative: bool
    published: bool
    live: bool


# One reserve, write, publish or rollback, and its acknowledgement.
#
# ``rejected`` is separate from ``accepted`` because a refused publication is not
# a failed one: an incompatible descriptor must leave every visible field
# untouched and say why, so a caller can tell "not compatible" from "not found".
@ac.struct
class StatusRequest:
    operation: ac.u8
    txn_key: ac.u32
    requester: ac.u8
    request_id: ac.u16
    response_route: ac.u8
    owner: ac.u16
    tile_version: ac.u16
    allocation_generation: ac.u16
    dtype: ac.u8
    layout: ac.u8
    shape: ac.u32
    valid_region: ac.u32
    allocation_mask: ac.u64
    initialized_mask: ac.u64
    fault_code: ac.u8
    accepted: bool
    matched: bool
    rejected: bool
    exhausted: bool
    published: bool


# One immutable status snapshot.
#
# ``contents_defined`` is computed from the two masks rather than stored, so it
# cannot disagree with them.  A stored copy would be a second owner of the same
# fact.
@ac.struct
class StatusQuery:
    requester: ac.u8
    request_id: ac.u16
    response_route: ac.u8
    owner: ac.u16
    tile_version: ac.u16
    dtype: ac.u8
    layout: ac.u8
    shape: ac.u32
    valid_region: ac.u32
    allocation_mask: ac.u64
    initialized_mask: ac.u64
    fault_code: ac.u8
    found: bool
    speculative: bool
    published: bool
    contents_defined: bool


# ---------------------------------------------------------------------------
# CHK — checkpoints over RAT's speculative history.
#
# A checkpoint is a frontier: how deep RAT's history stack was when the
# checkpoint was taken.  It is a number, not a copy of the map, because RAT
# already retains every displaced mapping; copying them here would make two
# owners of one fact and bound recovery by CHK's capacity instead of RAT's.
CHECKPOINT_SLOTS = 16

# Closed operations.  Release exists because the pool is bounded and a resolved
# branch must give its record back; without it a correctly predicted program
# would exhaust the pool.
CHK_CAPTURE = 0
CHK_UNWIND = 1
CHK_RELEASE = 2
# An operation outside the closed set, refused on its own port rather than routed
# out of range.
CHK_UNKNOWN = 3
CHK_BRANCH_MASK = 3


# One captured frontier.
#
# ``frontier`` is RAT's ``history_depth`` at capture, which the requester reads
# from the swap acknowledgement that preceded it.  CHK does not observe RAT: a
# module that read a sibling's state would be a second owner of it, so the one
# number CHK needs travels in the request.
@ac.struct
class Checkpoint:
    flow_key: ac.u32
    checkpoint_id: ac.u16
    frontier: ac.u6
    live: bool


# One capture, unwind or release, and its acknowledgement.
#
# ``steps`` is how many rollbacks RAT must accept to reach this checkpoint. It is
# the answer to "how far back", not a sequence of commands: chk.md records why the
# stepping itself is not owned here.
@ac.struct
class CheckpointRequest:
    operation: ac.u8
    flow_key: ac.u32
    checkpoint_id: ac.u16
    requester: ac.u8
    request_id: ac.u16
    response_route: ac.u8
    current_frontier: ac.u6
    target_frontier: ac.u6
    steps: ac.u6
    accepted: bool
    matched: bool
    exhausted: bool
    stale: bool
