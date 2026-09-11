# TMU.TRF.REF — Reference Count

- Source candidate: `DAV-TMU-TRF-REF-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trf/ref.py`
- Current design-program execution status: **source implementation present; gfsim
  verification pending**. Storage is a persistent indexed variable, which
  Decision 0151 classifies as provisional state, so PYC and RTL must reject this
  leaf at an explicit boundary. One behavioral acceptance is **not met** and is
  recorded as a framework gap below.

REF counts how many consumers still hold a Tile version, so its storage is not
taken back while somebody is still reading it.

## What problem it solves

One Tile version can be read by several consumers at once. Until the last of them
is finished, that physical space must not be reclaimed. REF is the ledger that
keeps that count.

The important half is what REF does *not* do: **it only counts, it does not decide
reclaim.** A count reaching zero is a signal that reclaim may now be considered,
nothing more. Actually taking the space back additionally needs logical-lifetime
permission, which REF cannot see.

A lease is named by `(owner, tile_version, lease_kind)`. The slot that holds it is
REF's private layout and appears in no response. Four operations cover the whole
job: acquire, transfer, release, and a read-only query.

## Where it sits

A Tile's lifetime is divided among several modules. REF owns exactly one part of
it -- how many readers are still there:

| Module | Owns | Does not own |
| --- | --- | --- |
| [RAT](../trn/rat.md) | Logical name to version mapping | Where that version's data lives |
| [FRE](../trn/fre.md) | Which physical block is free, and which block a version holds | What is in the block |
| [STS](../trn/sts.md) | A version's descriptor, definedness and publication status | The data bytes |
| [BANK](bank.md) | The data bytes themselves | Who owns the block, or when it may be reclaimed |
| **REF** | **How many readers still hold a version** | **Whether reclaim is actually permitted** |
| [LTR](../trn/ltr.md) | Sequencing the siblings above into one ordered transaction | Any of their state |

So REF sees neither the bytes nor the logical name. It sees a count. FRE is the
one that frees a slot, and it needs two conclusions before it does: zero physical
references, which is REF's answer, and logical-lifetime permission, which is not.
The coordinator that holds both is [ltr.md](../trn/ltr.md).

The BGF side of the fabric never appears here at all: [MAP](../bgf/map.md)
resolves a `cell_key` into a `(bank, row)` pair, [ARB](../bgf/arb.md) grants a
bank to one source class per cycle, and [RQ](../bgf/rq.md) / [WQ](../bgf/wq.md)
queue those accesses. Those modules move bytes; REF only accounts for readers.

## How one operation completes

The three mutating operations arrive on one stream and are told apart by
`operation`. The query rides its own stream.

**Acquire.** REF first looks for an existing lease with this
`(owner, tile_version, lease_kind)`. If it finds one, the count goes up by one. If
not, REF claims a free row and sets the count to 1. So one owner holding a version
twice is one row with a count of two, not two rows -- which is also what keeps the
search unambiguous, since a free row is claimed only when the search misses, so at
most one live row ever matches those three fields. A count already at its maximum
does not increment: a wrapped count would read as zero and make a live version
look reclaimable, so the acquire is refused instead.

**Transfer.** `owner` is rewritten in place. That is precisely why a transfer has
no unowned interval -- the row never stops existing. Moving the lease to a
different row would create exactly the window this forbids.

**Release.** The count goes down by one. When it reaches zero the row is marked no
longer live, `release_pending` is set, and this completion leaves through the
reclaim branch rather than the ordinary acknowledgement. A duplicate release is
safe without any extra comparison: the held-lease search requires `live`, so a
second release of an already-released lease finds nothing and changes nothing.

**Query.** A separate read-only rule reads the same ledger and never writes it, so
a query observes real state rather than a cached copy and never waits behind a
mutation.

Every mutation produces an acknowledgement, including one that changed nothing, so
no request is ever silently dropped. `accepted` reports whether the ledger changed,
`matched` whether the lease existed, and `count` the committed count observed.

Those bits do not separate every outcome. A full ledger, a saturated count and a
release of a lease that never existed all answer `accepted=False`. `matched` splits
off the saturated case, but a full ledger and a nonexistent lease share
`matched=False` while meaning entirely different things. See the open decisions.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| request | LeaseRequest | One acquire, transfer or release, distinguished by `operation` and qualified by `(owner, tile_version, lease_kind)` | implemented seam |
| query | RefcountQuery | Physical references held for a TileVersion; mutates nothing | implemented seam |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| acknowledged | LeaseRequest | Outcome of any mutation. `accepted` reports whether the ledger changed, `matched` whether the lease existed, `count` the committed count observed | implemented seam |
| reclaim | LeaseRequest | The release that reached zero references. Requires separate logical-lifetime permission; REF frees nothing | implemented seam |
| answered | RefcountQuery | `found` and `count` for the queried version | implemented seam |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Ledger array of `LEASE_SLOTS` rows, one lease per row, with exactly one writer
- Per-row owner, TileVersion, lease kind, reference count, acquired epoch,
  release-pending and tombstone
- No payload bytes, no descriptor state and no lifetime authority

## Reference implementation

[`ref.py`](ref.py) defines the mutating rule `serve_lease_request`, the read-only
rule `answer_refcount_query`, and the system `trf_ref_system`. Types and frozen
constants are shared through
[`contracts/tmu_trf.py`](../../contracts/tmu_trf.py).

One ledger array is elaborated with exactly one writer, which Decision 0151
requires. One row is one lease, and `count` is its reference count.

Two searches locate the work. The held-lease search requires `live`, `owner`,
`tile_version` and `lease_kind` to match together, so no single field can carry a
match alone. The free-slot search requires a row to be neither live nor
tombstoned: a tombstoned row still has late responses to drain, and reusing it
would let a drained response land on a different lease.

One rule serves all three mutating operations, which is what makes the ledger
single-port: a rule admits one token per tick, so at most one mutation reaches the
array per cycle. The acknowledgement stream is then split on `outcome` by an
ordinary route, so a caller waiting for acknowledgements and a reclaim consumer
are independently backpressured.

## Departures from the original proposal

**The three mutating operations share one stream** instead of the three request
ports this card originally proposed. That is forced, not preferred: a rule fires
only when *every* input has a token (Decision 0167/0168), and acquire, transfer
and release arrive independently, so three ports would deadlock waiting for each
other. One stream is also the honest physical model, since a single-port ledger
serves one operation per cycle. A query mutates nothing, so it keeps its own
stream and its own read-only rule.

**The reclaim candidate carries `LeaseRequest`** rather than its own payload type.
One rule cannot both search the table and produce several outputs: `ac.find`
combined with multiple outputs is rejected by the frontend, and Decision 0210
records multiple selected outputs as follow-up work. A queue transform cannot
change a payload type either, so the outcome is marked in the record and split by
an ordinary route. This is also faithful to the timing: zero references *is* an
outcome of the release that caused it, observed in the same tick.

**No response carries the physical slot.** A lease is named by
`(owner, tile_version, lease_kind)`; the slot is REF's private layout, for the
same reason `BankAccess` exposes no sub-cell coordinate.

**`reclaim` has no consumer yet.** Zero references is only one of the two
preconditions [fre.md](../trn/fre.md) requires before it releases a slot; the
other is logical-lifetime permission, which REF cannot see. The coordinator that
sees both is [ltr.md](../trn/ltr.md), which is not implemented, so this output
currently terminates. FRE is the eventual executor, not the direct receiver.

## Frozen decisions

This card listed lease kinds, table capacity, generation wrap bound and reduction
implementation as open. The implementation freezes them:

| Decision | Value | Reason |
| --- | --- | --- |
| Lease kinds | `LEASE_KIND_READ`, `LEASE_KIND_WRITE` | A reclaim candidate must not appear while a writer still holds the version, so the two are counted separately |
| Table capacity | `LEASE_SLOTS = 64` | The free-slot search makes this a parameter rather than an assumption baked into the logic |
| Wrap bound | `LEASE_COUNT_MAX`, saturating | A wrapped count would read as zero and make a live version look reclaimable, so an overflowed acquire is refused instead |
| Reduction implementation | None; one row per lease | No reduction operator exists, so a per-version query reads a row. See the limit in "Open decisions" |

## Backend boundary

Persistent indexed variables are provisional state under Decision 0151, and
PYC/RTL must reject them at an explicit boundary, so gfsim is the only backend
that can execute this leaf. [bank.md](bank.md) records the same boundary and the
framework work that would remove it.

## Required capabilities to verify

- Associative Table with field updates — available; `ac.find` with `where`/`key`
  locates a row and `with_fields` replaces it
- Atomic insert/increment and transfer across rows — available; two writes under
  one condition commit together. Transfer needs no second row, see below
- Multiple outputs and no-fail commit groups — **partly available**; a rule that
  searches cannot also produce several outputs, so the outcome is routed instead
- Reduction/query over generation-qualified entries — **unavailable**; no
  reduction operator exists. The per-row design avoids needing one for the common
  case, but not for every case; see "Open decisions"
- PYC/RTL Table lowering — **rejected by design** while storage is provisional
  state; see "Backend boundary"

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Acquire stalls before upstream acceptance when table is full — **not met**; see
  the first open decision. The leaf answers with `accepted=False` instead, so no
  request is dropped, but the requester must retry
- Transfer has no unowned interval — **holds**; ownership is rewritten in place,
  so the row never stops existing. Moving the lease to another row would create
  exactly the interval this forbids
- Duplicate release decrements once — **holds structurally**; idempotence comes
  from the search rather than a comparison, because the held-lease predicate
  requires `live`, so a second release finds nothing and changes nothing
- Zero count emits reclaim candidate but does not free logical version —
  **holds**; the only effects are the ledger write and the routed outcome. REF has
  one state owner and no free path
- Canceled reads retain tombstones until late responses drain — **partly**; a
  tombstoned row is excluded from the free-slot search, so it cannot be reused.
  Who sets and clears the tombstone has no port in this card; see "Open decisions"

Design-local evidence:
[`test_ref_lease_ledger.py`](../../tests/fabric/test_ref_lease_ledger.py).

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- **A full ledger cannot stall its requester, and that acceptance is unmet.**
  Decision 0210 fixes a rule's consume condition to a proven constant-true
  candidate and states the false output path *may consume input*, so a
  state-derived "table is full" condition cannot hold a request in its queue.
  The leaf therefore always answers, with `accepted=False`; nothing is dropped,
  but back-pressure becomes retry. Closing this needs state-qualified consumption
  in the framework, which is a reusable capability and belongs upstream. The
  design-local test pins the current behavior so it fails the day that lands.
- **A per-version refcount is one row's count, not a sum across rows.** When
  several owners hold the same version, the query reports the row with the most
  references rather than their total, because no reduction operator exists. The
  per-row design makes this correct for a single owner and wrong for shared
  versions. Either shared versions are excluded by an upstream invariant, or a
  reduction capability is required; neither is recorded yet.
- **One `accepted` bit does not separate a shortage from a logic error.** A full
  ledger, a saturated count and a release of a lease that never existed all answer
  `accepted=False`. `matched` isolates the saturated case, but a full ledger and a
  nonexistent lease share `matched=False` while meaning "retry later" and "the
  caller is wrong or repeating itself" respectively. A distinct reason code would
  make the difference visible, at the cost of a field on the shared record; which
  callers actually need to tell them apart is not established.
- **Tombstone has no producer or consumer port.** The field exists and excludes a
  row from reuse, which is what the acceptance needs, but this card lists no input
  that sets it on cancellation and none that clears it when late responses drain.
  The cancel/drain owner has to be identified before the lifecycle is complete.
- Freeze whether `acquired_epoch` participates in lease identity. It is currently
  recorded and refreshed on acquire but never compared, so a wrapped epoch cannot
  yet alias a lease.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [ ] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [ ] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [ ] Implement the accepted owner and design-local expected-result tests.
- [ ] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:250` — Detailed REF packet.
- `docs/architecture/core/l3/CONTRACT_FOUNDATION.md:104` — REF owns physical Tile lease ledger and explicit release Queue pairs.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
