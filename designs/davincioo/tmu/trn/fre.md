# TMU.TRN.FRE — Free-list

- Source candidate: `DAV-TMU-TRN-FRE-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRN`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trn/fre.py`
- Current design-program execution status: **source implementation present; gfsim
  verification pending**. One behavioral acceptance is not met; the cause is a
  framework limit recorded under "Open decisions".

FRE allocates physical tile storage: which block is free, which block a version
holds, and when it can be taken back.

## What problem it solves

A Tile instruction names a logical register -- "write tile 3" -- but the data has
to land somewhere physical in BANK. Who decides where this write goes, and which
blocks are still unused? That is FRE.

It keeps one ledger: how many physical tile blocks a PE has, which are taken, and
which are free. Anything writing a new version asks FRE for a block first. Once
the version is finished with, and nobody is still reading it, the block goes back.

There is a second layer. The core executes speculatively and a branch can resolve
the wrong way, so asking for a block cannot be a one-shot action; it has to be
undoable. FRE therefore splits allocation in two: reserve first, then either
commit or cancel. A cancel returns the block untouched, as if it had never been
requested.

## Where it sits

A Tile's lifetime is divided among several modules. FRE owns exactly one part of
it -- which physical block belongs to whom:

| Module | Owns | Does not own |
| --- | --- | --- |
| [RAT](rat.md) | Logical name to version mapping | Where that version's data lives |
| **FRE** | **Which physical block is free, and which block a version holds** | **What is in the block, or whether the version was ever written** |
| [STS](sts.md) | A version's descriptor, definedness and publication status | The data bytes |
| [BANK](../trf/bank.md) | The data bytes themselves | Who owns the block, or when it may be reclaimed |
| [REF](../trf/ref.md) | How many readers still hold a version | Whether reclaim is actually permitted |
| [LTR](ltr.md) | Sequencing the siblings above into one ordered transaction | Any of their state |

So FRE knows neither the contents nor whether a version means anything; it knows
only that a block is taken. The reverse holds too: nobody else decides block
ownership. Allocation has exactly one owner, and [alc.md](../trf/alc.md) records
why the candidate that looks like an allocator folds into this one.

## How one operation completes

All four operations arrive on one request stream and are told apart by
`operation`.

**Allocate.** The request carries a transaction key, `txn_key`. FRE looks that key
up first: if this transaction already took a block, it reports the same slot again
rather than handing out a second one. Rename can re-issue a request after a
pipeline replay, and this lookup is what keeps a replay from silently burning
storage. On a miss, FRE takes a free slot and a reservation row together, both
committing on the same tick. The block is now *reserved*, and still undoable.

Note what a repeated allocate answers: `slot_index` is the slot it already holds
and `matched` is set, but `accepted` is **false**, because `accepted` reports
whether the ledger changed and a duplicate changes nothing. A caller must read
`matched` to tell "you already have this slot" from `exhausted`, which is the real
failure. Reading `accepted` alone would misread a successful retry as a refusal.

**Commit.** The branch resolved the right way and this version is going to live.
FRE retires the reservation row. From here the block returns only through a free;
a cancel no longer does anything.

**Cancel.** The branch was wrong, or the instruction was flushed. FRE retires the
reservation row and marks the slot free again. Because the reservation lookup
requires a live row, a cancel after a commit finds nothing and changes nothing --
repeating a cancel is safe.

**Free.** The version is finished with for good. FRE cannot decide this on its own:
it cannot see how many readers remain, which is REF's ledger, and it cannot see
whether a logical name still refers to the version, which is RAT's side. So the
request must carry both conclusions. FRE checks that both hold, finds the slot by
owner and version, and marks it free.

Each operation's acknowledgement leaves on its own port. A capacity query is a
fifth path, read-only, and does not mix into the four above.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| request | AllocRequest | One allocate, commit, cancel or free, distinguished by `operation` and keyed by `txn_key` | implemented |
| query | CapacityQuery | Whether the pool still has a free slot; mutates nothing | implemented |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| alloc_ack | AllocRequest | Allocation result. `slot_index` names the slot; `accepted` reports whether the ledger changed, so a repeated transaction sets `matched` with `accepted` false; `exhausted` reports a full pool | implemented |
| commit_reservation_ack | AllocRequest | Reservation retired; the version is now live and only a free may release it | implemented |
| cancel_reservation_ack | AllocRequest | Reservation retired and its slot returned | implemented |
| free_version_ack | AllocRequest | Slot released, or refused when a precondition is missing | implemented |
| answered | CapacityQuery | `has_free_slot` for the queried pool | implemented |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- A slot pool of `TILE_SLOTS_PER_POOL` entries, one entry per physical tile slot,
  with exactly one writer. Instantiated per PE for Local and once per core for
  Shared
- Per slot: owner, TileVersion, allocation generation, and the allocated bit
- A reservation table of `RESERVATION_SLOTS` rows keyed by `txn_key`, holding
  in-flight cancellable reservations only
- No payload bytes, no descriptor, no logical name, and no lifetime authority

## Reference implementation

[`fre.py`](fre.py) defines the mutating rule `serve_allocation`, the read-only
rule `answer_capacity_query`, and the system `trn_fre_system`. Types and frozen
constants are shared through
[`contracts/tmu_trn.py`](../../contracts/tmu_trn.py).

The four operations above are located by four searches. The duplicate search
requires a live reservation row with a matching `txn_key`. The free-slot and
free-row searches are the two resources an allocate needs at once, so both writes
are guarded by one condition -- that is how "all-or-none" is implemented. The
target search matches owner and version, because that is how a free arrives: the
reclaim chain names a version and never a slot.

One rule serves all four operations, which is what makes the allocator
single-port: a rule admits one token per tick, so at most one operation reaches
the pool per cycle. Each array has exactly one writer, as Decision 0151 requires.
The query is a second rule that reads the same pool and never writes it, so a
query observes real state rather than a cached count and never queues behind a
mutation.

## Departures from the original proposal

Three things differ from what this card first proposed. The first two are forced
by the framework; the third is an architectural judgement.

**The four mutating operations share one stream instead of four request ports.** A
rule fires only when *every* input has a token (Decision 0167/0168), and allocate,
commit, cancel and free arrive independently, so four ports would deadlock waiting
for each other. One stream also matches the hardware, since a single-port
allocator serves one operation per cycle. Acknowledgements still leave on separate
ports, via a plain route keyed on `operation`.

**Local and Shared share one slot geometry, and Shared is a separate instance.** A
slot index is exactly as wide as its pool needs and the frontend has no width
conversion primitive, so two pools of different capacity could not share one
request record. Shared also cannot sit inside the per-PE instance: it is
core-owned, and four per-PE rules writing one Shared pool would give that Table
four write endpoints, which Decision 0151 forbids. `scope` is carried for the
parent, which uses it to select an instance; FRE never reads it, for the same
reason BANK never reads `cell_key` -- whoever routes on a field must be the only
one interpreting it.

**The physical resource is a fixed set of fixed-size slots, not a heap.** This card
says "extent", which suggests a variable length. But TRF is a register *file*, so
fixed slots are what the hardware is, exactly as a physical register pool is. The
framework confirms the same judgement independently: a variable-length extent
needs a contiguous-run search across table entries, and there is no reduction or
run-length primitive, whereas selecting one fixed-size free slot is a single
`ac.find`. Slot geometry is derived from the BANK and BGF contracts so a pool
covers exactly one PE's cells and the two cannot drift.

## Frozen decisions

This card listed extent allocation policy, physical capacities and reset-image
construction as open. The implementation freezes them:

| Decision | Value | Reason |
| --- | --- | --- |
| Allocation unit | One fixed-size slot | A variable extent needs a contiguous-run search, which no primitive provides |
| Pool capacity | `TILE_SLOTS_PER_POOL = 64` | The free-slot search makes this a parameter, not an assumption baked into the logic |
| Slot size | Derived: `CELLS_PER_PE / TILE_SLOTS_PER_POOL` | Imported from BANK/BGF so a pool covers exactly one PE's cells |
| Reset image | All-zero; a slot stores `allocated`, not `free` | Decision 0151 admits an all-zero image only, so this meets the nonzero-reset requirement without a reset FSM |
| Reservation capacity | `RESERVATION_SLOTS = 32` | Bounds concurrent rename transactions, not live versions, since a row retires at commit |
| Capacity answer | A bit, not a count | No reduction operator exists; see "Open decisions" |

## Backend boundary

Persistent indexed variables are provisional state under Decision 0151, which
PYC/RTL must reject at an explicit boundary, so gfsim is the only backend that can
execute this leaf. [bank.md](../trf/bank.md) records the same boundary and the
framework work that would remove it.

## Required capabilities to verify

- Parameterized banked bitset/Array state — **available**; a pool is a persistent
  indexed variable of flat structs (Decision 0151)
- Bounded extent selection — **available for one fixed-size slot**; `ac.find` with
  `where`/`key` selects a free entry. A contiguous run of entries is **not**
  selectable, which is why the allocation unit is one slot
- Associative idempotence Table — **available**; the duplicate search keyed on
  `txn_key` makes a repeated transaction return its existing slot
- Atomic reserve plus response and cancel/free — **available**; two writes under
  one condition commit together, and four searches coexist in one rule
- Nonzero reset initialization FSM or verified primitive — **not needed**; storing
  `allocated` instead of `free` makes the all-zero image mean "every slot is
  available"
- PYC/RTL Table lowering — **rejected by design** while the storage is provisional
  state; see "Backend boundary"

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Allocation is all-or-none — **holds**; the slot write and the reservation write
  are guarded by one condition, so a slot can never exist without its row
- Duplicate RenameTxnKey returns the same reservation — **holds**; the duplicate
  search runs before any allocation and its result, not a fresh slot, is reported
- PE-local exhaustion does not consume another PE's pool — **holds structurally**;
  the pool is declared inside the system, so each instance owns a separate pool and
  no port exposes one
- Free requires both logical permission and zero REF count — **holds**; both bits
  ride the request and are checked, and the target search additionally requires an
  allocated slot matching owner and version
- Physical capacity exhaustion backpressures rather than inventing an architectural
  fault — **not met**; see the first open decision. FRE invents no fault and drops
  nothing: it answers with `exhausted` and `accepted=False`. But it consumes the
  request instead of holding it, so backpressure degrades to retry

Design-local evidence: `designs/davincioo/tests/fabric/test_fre_slot_allocator.py`.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- **An exhausted pool cannot stall its requester, so that acceptance is unmet.**
  Decision 0210 fixes a rule's consumption condition to a proven-constant-true
  candidate and states that the false path **may consume input**, so a condition
  derived from pool state cannot hold the request in its queue. FRE therefore
  always answers, with `exhausted` set. Nothing is lost, but backpressure becomes
  retry. Closing this needs state-qualified consumption in the framework, which is
  a reusable capability and belongs upstream. This is the same gap REF hit, not a
  second one. The design-local test pins current behavior, so the assertion fails
  once the capability lands.
- **A capacity answer is a bit, not a count.** There is no reduction operator, so
  the number of free slots cannot be computed and `has_free_slot` reports only
  whether one exists. [alc.md](../trf/alc.md) folds capacity projection into this
  interface; this is how much of it FRE can answer. A caller needing a count either
  requires a reduction capability or must track allocation itself.
- **A slot index width is a literal, not a derived expression.** The frontend
  requires a static width in `ac.bits[...]` (`ACPY-TYPE-003`) because it parses
  annotations as source text, so `SLOT_INDEX_BITS` cannot size the field. An
  assertion keeps the literal and the pool capacity consistent, but changing the
  capacity requires editing both.
- **The Shared pool has no confirmed host.** It cannot live in the per-PE instance
  without giving one Table four write endpoints, so it is a separate instantiation
  of the same system. Whether the parent that owns it is TRN assembly or a
  core-level assembly is not recorded.
- **Allocation generation is carried but never compared.** It is stored on a slot
  and echoed on a release, so a wrapped generation cannot yet alias a stale free.
  Freezing whether it participates in the target search is open.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:221` — Detailed FRE packet.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:753` — Raw capacity and architectural quota are accounted separately.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
