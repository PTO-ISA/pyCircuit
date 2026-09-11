# TMU.TRN.CHK — Checkpoint

- Source candidate: `DAV-TMU-TRN-CHK-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRN`
- Recommended disposition: **leaf** (proposal, not registry approval). This card
  previously said **review** because ownership was unresolved between a standalone
  leaf and RAT-private state; see "Why this is a leaf".
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trn/chk.py`
- Current design-program execution status: **gfsim-verified**. The behavioral
  acceptances below run as real transactions against the generated model; see
  "Backend boundary". One acceptance is owned elsewhere by design and one is
  unmet; both are recorded under "Open decisions".

CHK remembers where to go back to. A checkpoint is one number -- how deep
[RAT](rat.md)'s history was at a moment worth returning to.

## What problem it solves

The core executes speculatively, so it must be able to return the rename state to
an earlier point. [RAT](rat.md) already keeps everything needed for that: every
displaced mapping sits on its history stack, and a rollback pops one entry. What
RAT does not keep is *where to stop*.

A branch that may be mispredicted takes a checkpoint before it. If the prediction
was wrong, recovery rolls RAT's history back to the depth that checkpoint
recorded. So the whole content of a checkpoint is a frontier -- a stack depth --
plus the identity that names it.

That is the design's central economy: CHK stores a number, not a copy of the map.
Copying displaced mappings here would create a second owner of a fact RAT already
owns, and it would bound recovery by CHK's capacity rather than by RAT's history.

## Why this is a leaf

The open question on this card was whether the checkpoint mechanism should be a
module or private state inside RAT. It is a leaf, for one reason: the two pieces
of state answer different questions and have different lifetimes. RAT's history is
per displaced mapping and is consumed by the rollbacks that undo it. A checkpoint
is per speculation point, survives many swaps, and is released when a branch
resolves correctly -- which is the common case, where no rollback happens at all.

Folding them together would put both under one rule, so a capture and a swap would
contend for the same single-port owner even though they share no state. Keeping
them apart costs one seam and gives each owner one job.

What CHK does *not* get from being a leaf is the ability to drive recovery. It
never writes RAT, FRE or STS; it answers with a frontier and a distance, and the
rollbacks themselves are RAT transactions that RAT re-checks against its own stack
top. A recovery is therefore qualified twice, by two owners, and neither trusts
the other.

## Where it sits

| Module | Owns | Does not own |
| --- | --- | --- |
| [RAT](rat.md) | The map and the history stack that undoes it | Where a recovery should stop |
| **CHK** | **Captured frontiers, and which of them are still usable** | **The map, the history, and the stepping of a recovery** |
| [FRE](fre.md) | Physical blocks, and their cancellable reservations | Rename state of any kind |
| [STS](sts.md) | Version descriptors and publication status | Rename state of any kind |

## How one operation completes

Three operations arrive on one request stream and are told apart by `operation`.

**Capture.** The requester has just seen a swap acknowledgement from RAT, so it
knows the current history depth. It sends that depth with a checkpoint identity,
and CHK stores the pair. A repeated capture of the same identity returns the
record it already has instead of consuming a second row, so a replayed capture
cannot leak checkpoint capacity.

**Unwind.** Speculation was wrong. CHK looks the checkpoint up and answers with
the frontier to return to and the number of rollbacks that reach it -- the
distance between the requester's current depth and the recorded one. The record
stays live, so an unwind interrupted by backpressure can be re-attempted; a
retired record would make the second attempt look like an unknown checkpoint.

An unwind is refused as `stale` when the recorded frontier is *deeper* than the
map's current one. That single comparison is the entire staleness rule, and it is
worth seeing why it is enough: a checkpoint captured after the point being
recovered to necessarily has the deeper frontier, so once recovery finishes it
fails this test forever. Nothing has to sweep the table to invalidate the
checkpoints that recovery just made meaningless.

**Release.** The branch resolved correctly, so the record goes back. This exists
because the pool is bounded and correct prediction is the common path; without a
release, a program that never mispredicts would still exhaust the pool.

Each acknowledgement leaves on its own port. A fourth port carries an operation
outside the closed set, refused there.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| request | CheckpointRequest | One capture, unwind or release, distinguished by `operation`; carries the requester's `current_frontier` | implemented |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| checkpoint_ack | CheckpointRequest | Frontier captured, or the existing record for a repeated capture; `exhausted` reports a full pool | implemented |
| unwind_ack | CheckpointRequest | `target_frontier` is where recovery stops and `steps` is how many RAT rollbacks reach it; `stale` reports an unusable checkpoint | implemented |
| release_ack | CheckpointRequest | Record retired | implemented |
| refused | CheckpointRequest | An operation outside the closed set, answered rather than routed out of range | implemented |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- A checkpoint pool of `CHECKPOINT_SLOTS` records, with exactly one writer,
  declared inside the system so each rename scope owns its own
- Per record: flow key, checkpoint identity, captured frontier, and live
- No map rows, no history entries, no unwind cursor, and no sibling state of any
  kind

## Reference implementation

[`chk.py`](chk.py) defines the rule `serve_checkpoint_request` and the system
`trn_chk_system`. Types and frozen constants are shared through
[`contracts/tmu_trn.py`](../../contracts/tmu_trn.py).

Two searches locate the work: the record this request names, and a free row for a
new capture. One rule serves all three operations, which keeps the pool
single-port and its writer single, as Decision 0151 requires.

An unwind writes nothing. That is visible in the lowered module as exactly two
write conditions -- capture and release -- neither of which is the unwind
condition, which is what `test_an_unwind_reports_a_distance_rather_than_driving_it`
asserts. It is the strongest available form of "no direct mutation of sibling
state": there is no sibling state to mutate, because none is reachable.

## Departures from the original proposal

**CHK does not own an unwind phase.** This card lists "history frontier and unwind
phase if separate leaf" as owned state. The frontier is owned here; the phase is
not, and this is a framework limit rather than a preference.

Emitting one rollback step per cycle needs an owner that both registers an unwind
from a request *and* advances a cursor without one. A rule fires either on a token
or on state, not both. Splitting it into two rules -- one registering, one
draining -- would give the cursor two writers, which Decision 0151 forbids; the
draining rule alone is expressible, but it cannot learn that a new unwind started
without reading state its sibling rule writes and then clearing it, which is the
same second-writer problem one step removed.

So recovery is driven by the caller: CHK says how far, RAT pops one entry per
rollback. Nothing is lost in correctness -- RAT's stack still enforces
youngest-first and re-checks each generation -- but the sequencing is not a
hardware FSM in this leaf, and a profile that requires one needs the framework
capability first.

**Staleness is derived rather than swept.** Invalidating the checkpoints that a
recovery made meaningless would need a masked multi-row update, which this storage
does not have: a search writes one row. The frontier comparison makes the sweep
unnecessary, which is a better outcome than the sweep would have been, but it is
worth recording that the alternative was not available.

**A release operation was added.** The card lists capture and unwind only. A
bounded pool with no way to give a record back fills up on the correctly predicted
path, which is the common one, so release is required for the mechanism to work at
all rather than being an extra feature.

**A fourth acknowledgement port exists for an operation outside the closed set.**
Three operations do not fill a two-bit branch, and a route selector outside
`[0, outputs)` is a deterministic runtime failure, so a malformed request is
refused on a port rather than failing the model.

## Frozen decisions

| Decision | Value | Reason |
| --- | --- | --- |
| Disposition | Leaf | Checkpoints and history answer different questions with different lifetimes; see "Why this is a leaf" |
| Checkpoint content | One frontier plus identity | RAT already retains the displaced mappings; copying them would create a second owner |
| Pool capacity | `CHECKPOINT_SLOTS = 16` | Bounds concurrent speculation points; the row search makes it a parameter |
| Frontier source | Carried in the request | CHK does not observe RAT; a module reading a sibling's state would be a second owner of it |
| Staleness | `current_frontier >= frontier` | A checkpoint taken after the recovery point has the deeper frontier, so one comparison replaces a sweep |
| Reset image | All-zero; a record stores `live` | Decision 0151 admits an all-zero image only |

## Backend boundary

Persistent indexed variables are provisional state under Decision 0151, which
PYC/RTL must reject at an explicit boundary, so gfsim is the only backend that can
execute this leaf. [bank.md](../trf/bank.md) records the same boundary and the
framework work that would remove it.

CHK reaches that backend. `ac.jit(trn_chk_system).lower_acir()` lowers through
`acir-opt` and `acir-queue-cxxgen` into a typed `gfsim::QueueTableTransition` over
the checkpoint pool, which the design-local gfsim test compiles and runs. Both
tools are built from `compiler/acir` with the `dev-llvm22` preset; the test skips,
rather than fails, when they or a C++20 compiler are absent.

Its sibling owners do not reach it yet, and the reason is not their storage:
[rat.md](rat.md) and [sts.md](sts.md) record the read-only-query-rule gap that
stops C++ generation for every owner that answers queries beside its writer. CHK
generates precisely because it has no query rule.

## Required capabilities to verify

- Bounded Table/stack semantics — **available**; the pool is a persistent indexed
  variable and the ordering it needs lives in RAT's stack, not here
- Multi-cycle acknowledged recovery transaction — **not available**; a rule fires
  on a token or on state, not both, so the stepping is not owned here. See
  "Departures"
- Atomic coordination with RAT/FRE/STS through parent Queues — **available**; the
  answer is a transaction on this leaf's own state and the recovery it describes is
  a sequence of RAT transactions
- PYC/RTL Table lowering — **rejected by design** while the storage is provisional
  state; see "Backend boundary"

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Recovery unwinds youngest-first — **holds, owned by RAT**; CHK reports a target
  and a distance, and RAT pops its stack top for each rollback, so ordering is the
  shape of RAT's storage. CHK cannot violate it and does not enforce it
- A stale checkpoint cannot restore a newer mapping — **holds**, and twice over:
  CHK refuses an unwind whose recorded frontier is deeper than the current one, and
  RAT independently refuses a rollback whose generation does not match its stack
  top
- No direct mutation of sibling RAT/FRE/STS state — **holds structurally**; the
  module declares one owner and consumes one request stream, so no sibling state is
  reachable

Design-local evidence: `designs/davincioo/tests/fabric/test_chk_checkpoint_stack.py`
asserts the structure on the lowered module, and
`designs/davincioo/tests/fabric/test_chk_gfsim.py` executes the acceptances above
in gfsim: a capture, a repeated capture, an unwind's distance, a stale unwind, a
release, an unknown operation, and an exhausted pool.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- **The unwind phase has no owner.** A hardware sequencer that emits one rollback
  per cycle needs a rule that fires on both a request and its own state, which the
  framework does not provide without giving one cursor two writers. Until that
  capability exists, recovery stepping belongs to the caller. This is a real
  capability gap and belongs upstream, not a design preference.
- **A full pool cannot stall its requester.** Decision 0210 fixes a rule's
  consumption condition to a proven-constant-true candidate, so a capture that
  finds no free row answers with `exhausted` rather than being held. This is the
  same gap FRE, REF, RAT and STS record.
- **Checkpoint identity allocation has no owner.** `checkpoint_id` and `flow_key`
  arrive in the request and are compared, never generated. Who allocates a unique
  identity, and what happens if one is reused while its record is still live, is
  unresolved.
- **A frontier is trusted.** `current_frontier` comes from the requester, which
  read it from a RAT acknowledgement. If the requester is wrong, the distance is
  wrong. CHK cannot check it without observing RAT, which would make it a second
  owner of RAT's depth, so the trust is deliberate but should be stated in the
  parent's contract.
- **The relationship to FRE's reservations is unstated.** A recovery that rolls
  the map back must also cancel the reservations those swaps took. Nothing here
  sequences the two, and the coordinator that must is [LTR](ltr.md), which is not
  implemented.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [x] Resolve disposition; aliases and contained state must not duplicate hardware.
      Resolved as a leaf: see "Why this is a leaf".
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [ ] Decide who allocates checkpoint identities and what a reused identity means.
- [ ] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for the unwind-phase capability gap and merge that shared fix first.
- [ ] Implement the accepted owner and design-local expected-result tests.
- [ ] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:113` — Disposition explicitly remains leaf or RAT-private state.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:216` — RAT speculative history stalls and unwinds youngest-first.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
