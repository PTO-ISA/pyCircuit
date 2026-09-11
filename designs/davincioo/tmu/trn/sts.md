# TMU.TRN.STS — Tile Status

- Source candidate: `DAV-TMU-TRN-STS-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRN`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trn/sts.py`
- Current design-program execution status: **source implementation present; gfsim
  verification pending**. One behavioral acceptance is not met; the cause is a
  framework limit recorded under "Open decisions".

STS answers what a version *is*: its descriptor, how much of it has actually been
written, what went wrong with it, and whether it is architectural yet.

## What problem it solves

[FRE](fre.md) hands out a physical block and [RAT](rat.md) points a name at a
version, but neither knows anything about the version itself. What shape is it?
What element type? Has anything been written into it, or is it allocated and
still garbage? Did a fault happen while producing it? Is it visible to
architecturally later instructions, or still speculative? That is STS.

The distinction that matters most is between *allocated* and *defined*. Getting
storage for a version says where it may be written; it does not say anything was.
A consumer reading a version that was allocated but never written would read
whatever the block held before. So STS keeps two separate coverage masks --
allocation and initialization -- and a version counts as defined only where the
second one covers the first.

## Where it sits

A Tile's lifetime is divided among several modules. STS owns exactly one part of
it -- what the version is and whether it is visible:

| Module | Owns | Does not own |
| --- | --- | --- |
| [RAT](rat.md) | Logical name to version mapping | Anything about the version itself |
| [FRE](fre.md) | Which physical block a version holds | Whether the block was ever written |
| **STS** | **Descriptor, coverage, first fault, and publication status** | **The data bytes, the name, and the block** |
| [BANK](../trf/bank.md) | The data bytes | Whether those bytes are meaningful |
| [REF](../trf/ref.md) | How many readers hold the version | Whether it is architectural |

Publication appears in two places on purpose and means two different things. RAT
publishes a *name*: from now on this name means this version. STS publishes a
*version*: this version's contents and descriptor are final. A coordinator does
both, and either alone is incomplete.

## How one operation completes

Four mutating operations arrive on one request stream and are told apart by
`operation`. A query rides its own read-only stream.

**Reserve.** A new speculative version appears. STS finds a free row and fills in
the descriptor and the allocation mask that the version is permitted to cover. The
initialization mask is written as zero, explicitly: the version has storage and no
contents. A reserve is refused when the row already exists, so the same version is
never described twice.

**Write.** A producer wrote part of the version. Writes arrive as several partial
updates, so coverage *merges* rather than replaces -- an update that replaced the
mask would erase the coverage of everything written before it. The same operation
carries the descriptor, and the first fault seen is retained: a row that already
holds a fault keeps it, so the fault a caller reads is the one that caused the
others rather than the last one reported. A write is qualified by allocation
generation and refused on a published row.

**Publish.** The version becomes architectural. This is the one operation that
validates before it changes anything: the descriptor in the request is compared
against the committed row, and an incompatible publication is refused with
`rejected` while every field stays as it was. There is no interval in which an
incompatible version is visible, because the compatibility test is part of the
write's condition rather than a correction applied afterwards.

**Rollback.** Speculation was wrong, so the speculative row is retired. Only a
speculative row of the same allocation generation can be reached: a published row
and a newer generation are both untouchable, which is what stops a stale recovery
from erasing architectural state.

**Query.** A read-only path reports the descriptor, both masks, the retained
fault, and whether the version is speculative or published. `contents_defined` is
computed from the two masks rather than stored, so it cannot disagree with them.

Each acknowledgement leaves on its own port, so a blocked reserve consumer cannot
stall a publish.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| request | StatusRequest | One reserve, write, publish or rollback, distinguished by `operation` and qualified by `allocation_generation` | implemented |
| query | StatusQuery | Read one version's status snapshot; mutates nothing | implemented |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| status_reserve_ack | StatusRequest | Speculative row created; `exhausted` reports a full table | implemented |
| status_write_ack | StatusRequest | Coverage merged and descriptor updated; `fault_code` is the retained first fault | implemented |
| status_publish_ack | StatusRequest | Version made architectural, or `rejected` when the descriptor was incompatible | implemented |
| status_rollback_ack | StatusRequest | Speculative row deleted, or refused when the row is published or of another generation | implemented |
| answered | StatusQuery | Descriptor, both masks, retained fault, and `contents_defined` | implemented |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- A status table of `STATUS_ROWS` rows keyed by `(owner, tile_version)`, with
  exactly one writer, declared inside the system so each rename scope owns its own
- Per row: owner, version, allocation generation, dtype, layout, shape, valid
  region, allocation mask, initialization mask, retained fault, speculative,
  published, and live
- No payload bytes, no logical name, no physical slot, and no reference count

## Reference implementation

[`sts.py`](sts.py) defines the mutating rule `serve_status_request`, the read-only
rule `answer_status_query`, and the system `trn_sts_system`. Types and frozen
constants are shared through
[`contracts/tmu_trn.py`](../../contracts/tmu_trn.py).

A row is located by *searching* on `(owner, tile_version)` rather than by
indexing, because version identifiers are sparse: a bounded number of versions are
live at once out of a 16-bit space. That is the opposite of RAT's map, where a
logical name is a dense index, and the two shapes are the reason those two modules
read their state with different primitives.

One rule serves all four operations, which is what makes the table single-port: a
rule admits one token per tick. The table has exactly one writer, as Decision 0151
requires. The query is a second rule reading the same table, so a query observes
real state rather than a cached snapshot and never queues behind a mutation.

The lowered module joins the write, publish and rollback row writes, since all
three target the located row and the operations are mutually exclusive; the
reserve keeps its own write because it targets a free row instead.

## Departures from the original proposal

**The four mutating operations share one stream instead of four request ports.** A
rule fires only when *every* input has a token (Decision 0167/0168), and a reserve,
a write, a publish and a rollback arrive independently, so four ports would
deadlock waiting for each other. Acknowledgements still leave on separate ports,
via a plain route keyed on `operation`. Four operations exactly fill a two-bit
branch, so unlike RAT and CHK no refusal port is needed.

**Element definedness is a 64-bit mask, not a general representation.** This card
left the bounded representation open. A mask is the widest value the payload
family admits, and there is no reduction operator, so "how much of this version is
defined" is answered by comparing two masks rather than by counting. A version
needing finer granularity needs a different representation, not a wider field.

**Descriptor fields are opaque scalars.** `dtype`, `layout`, `shape` and
`valid_region` are fixed-width values that STS compares for equality and never
interprets. Compatibility is therefore exact equality, not a subtyping or layout
rule. A profile needing "compatible but not identical" descriptors needs that
predicate specified before it can be implemented, since STS has no descriptor
semantics of its own.

**Publication is single-destination.** The card asks that a Shared
multi-destination publication cannot expose mixed old/new records. One rule
activation writes one row, so a publication spanning several destinations is
several operations and this leaf cannot make them atomic; see "Open decisions".

**Every operation writes one row through one selected write.** Two writes of one
owner must be provably disjoint or provably exclusive, and the analyzer cannot
derive either from independent operation guards, so a second write is rejected
with `same-owner proposals may select one index concurrently`. The index and the
value are selected instead, which makes the exclusion structural.
**A read-only query rule blocks C++ generation, so this owner has no gfsim
evidence yet.** The rule verifier accepts the design and `acir-opt` freezes it,
but `acir-queue-cxxgen` rejects the module with `table firing contract is
unsupported`. The cause is not the query itself: a firing that only *reserves* a
table, which is what a read-only rule over the writer's table lowers to, carries
no primary write, and the generator requires every stateful firing to name one.
The minimal reproduction is a sixteen-line system with one writing rule and one
reading rule over one table; deleting the reading rule generates. Three separate
places in `compiler/acir/lib/CodeGen/QueueGraphGenerator.cpp` assume a primary
write, so this is a framework capability gap and belongs upstream, not a
design-local workaround. [chk.md](chk.md) generates because it has no query rule.

## Frozen decisions

| Decision | Value | Reason |
| --- | --- | --- |
| Table capacity | `STATUS_ROWS = 64` | The row search makes this a parameter, not an assumption baked into the logic |
| Row key | `(owner, tile_version)` | Versions are sparse, so a row is found rather than indexed |
| Coverage | Two 64-bit masks, allocation and initialization | Allocation must not imply definedness, and one mask cannot express both |
| Definedness | Computed from the masks, never stored | A stored bit would be a second owner of a fact the masks already carry |
| Fault retention | First fault wins; `FAULT_NONE = 0` | Zero means "no fault", so the reset image and a fresh row agree without a reset pass |
| Reset image | All-zero; a row stores `live` | Decision 0151 admits an all-zero image only |
| Compatibility | Exact equality of dtype, layout and shape | STS has no descriptor semantics to reason with |

## Backend boundary

Persistent indexed variables are provisional state under Decision 0151, which
PYC/RTL must reject at an explicit boundary, so gfsim is the only backend that can
execute this leaf. [bank.md](../trf/bank.md) records the same boundary and the
framework work that would remove it.

## Required capabilities to verify

- Associative Table with partial field/mask updates — **available**; `ac.find`
  locates the row and an immutable field update writes part of it
- Atomic multi-Queue/Table events — **available**; the row write and the
  acknowledgement commit in one inferred transaction
- Bounded element-definedness representation — **available as a 64-bit mask**; see
  "Departures"
- Multiple independently backpressured outputs — **available**; the route gives
  each acknowledgement its own port and its own capacity
- Hierarchical PYC/RTL Table lowering — **rejected by design** while the storage is
  provisional state; see "Backend boundary"

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Allocation does not imply payload definedness — **holds**; the two masks are
  separate fields and a reserve installs a zero initialization mask, so the
  property is state rather than convention
- Blocked ack prevents request consumption and writes — **holds for output
  backpressure**; the inferred transaction reserves the selected output before any
  effect publishes, so a full acknowledgement queue leaves the table untouched.
  It does **not** hold for a full table; see the first open decision
- Publication validates descriptor compatibility before visible state changes —
  **holds**; compatibility is part of the write's condition, and an incompatible
  publication reports `rejected` while every field stays as it was
- Rollback cannot erase a newer published generation — **holds**; the guard
  requires a speculative, unpublished row of the same allocation generation
- Shared multi-destination publication cannot expose mixed old/new records — **not
  addressed**; publication here is one row per operation. Whether multi-destination
  publication is a single transaction or an ordered protocol is open
- STS contains no payload bytes — **holds**; the row declares descriptor, coverage
  and status fields only, which the design-local test asserts on the declaration

Design-local evidence: `designs/davincioo/tests/fabric/test_sts_status_table.py`.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- **A full table cannot stall its requester.** Decision 0210 fixes a rule's
  consumption condition to a proven-constant-true candidate and states that the
  false path **may consume input**, so a condition derived from table occupancy
  cannot hold the request in its queue. A reserve that finds no free row answers
  with `exhausted` instead. This is the same gap FRE, REF and RAT record, and
  closing it needs state-qualified consumption in the framework.
- **Multi-destination publication has no transaction.** One activation writes one
  row, so publishing several destinations atomically would need either a masked
  multi-row update on this storage or a protocol that stages then commits. Which
  one the architecture wants is not recorded, and the acceptance cannot be met
  until it is.
- **Descriptor compatibility is equality.** Whether a publication may legitimately
  narrow or reinterpret a descriptor is unresolved; if it may, the predicate has to
  be specified, since STS cannot infer it.
- **The descriptor snapshot form is still a scalar bundle.** The card asks for a
  snapshot or handle form. Four opaque scalars is the bundle; whether a handle into
  a descriptor store is preferable depends on how large a real descriptor is, which
  is not recorded anywhere in this program yet.
- **Coverage granularity is unstated.** A 64-bit mask covers 64 units, but what one
  unit *is* -- an element, a cell, a row -- is not frozen. [bank.md](../trf/bank.md)
  fixes the physical cell size, so the mapping between the two must be stated
  before the mask means anything precise.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [ ] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [ ] State the coverage granularity one mask bit represents.
- [ ] Decide whether multi-destination publication is one transaction or a protocol.
- [ ] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [ ] Implement the accepted owner and design-local expected-result tests.
- [ ] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/specification/davincioo/ndf-next/tile/tmu.md:36` — Normative descriptor/status fields and speculative ownership.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:159` — Detailed STS state and Queue packet.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:789` — Descriptor/version owner, not raw cells, retains metadata.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
