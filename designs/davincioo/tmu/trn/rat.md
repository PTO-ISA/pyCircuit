# TMU.TRN.RAT — Register Alias Table

- Source candidate: `DAV-TMU-TRN-RAT-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRN`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trn/rat.py`
- Current design-program execution status: **source implementation present; gfsim
  verification pending**. One behavioral acceptance is not met; the cause is a
  framework limit recorded under "Open decisions".

RAT answers one question: which physical version does a logical tile name mean
right now, and which version of that name is architectural.

## What problem it solves

A Tile instruction says "read tile 3, write tile 5". Those numbers are logical
names, and the core executes speculatively, so tile 5 may be written several times
by instructions that have not resolved yet. Each of those writes is a separate
physical version. Something has to say which version the name currently means,
and which one is real if the speculation is thrown away. That is RAT.

It keeps one map: for each logical name, the version it currently maps to, and the
last version of it that became architectural. A rename reads the map to find its
sources and installs a new version for its destination.

There is a second layer, and it is what makes recovery possible. Installing a new
version overwrites the only record of the previous one, so before overwriting,
RAT pushes the row it is about to displace onto a history stack. A rollback pops
that stack and puts the row back exactly as it was. Without the stack a
mispredicted branch could not be undone; with it, undoing is just popping.

## Where it sits

A Tile's lifetime is divided among several modules. RAT owns exactly one part of
it -- what a name means:

| Module | Owns | Does not own |
| --- | --- | --- |
| **RAT** | **Logical name to version mapping, and the history that undoes it** | **Where the version's data lives, or what is in it** |
| [FRE](fre.md) | Which physical block is free, and which block a version holds | What a name refers to |
| [STS](sts.md) | A version's descriptor, definedness and publication status | The name that reaches it |
| [CHK](chk.md) | Captured recovery points into RAT's history | The map itself; it never writes RAT |
| [BANK](../trf/bank.md) | The data bytes | Any of the above |

[LRM](lrm.md) is not a fifth owner. It is the name for RAT's Local projection, and
its disposition is explicitly an alias: a Local request is a `MapRequest` with
`scope = SCOPE_LOCAL`, answered by the per-PE instance. There is no second map.

## How one operation completes

Three mutating operations arrive on one request stream and are told apart by
`operation`. A lookup rides its own read-only stream.

**Swap.** A rename installs a new version for a logical name. RAT reads the row
that name currently holds, writes the new version in its place, and pushes the old
row onto the history stack. Both writes are guarded by one condition, so a mapping
is never overwritten without the record that can restore it. The acknowledgement
reports the displaced version and the new history depth -- the depth is what
[CHK](chk.md) captures as a checkpoint.

**Publish.** The instruction that produced a version retired, so that version
becomes architectural. RAT marks the row published and records the version as the
last published one. The request carries `map_generation` and the operation applies
only if it matches the row, so a publish that lost its race against a newer swap
does not promote the newer version by mistake.

**Rollback.** Speculation was wrong. RAT pops the newest history entry and
restores the complete row it holds: version, generation, last published version,
and whether the name was valid at all. That last part matters -- a name that had
no version before the swap must go back to having none, not to version zero. The
request carries the generation it expects to undo, and the operation applies only
if the stack top matches, so a stale recovery cannot undo work it never saw.

Because the history is a stack, repeated rollbacks proceed youngest-first with no
age comparison anywhere. Ordering is the shape of the storage.

**Lookup.** A read-only path reports a name's current version, its last published
version, and whether the current one is architectural. A consumer that must not
read speculative state decides here rather than asking STS.

Each acknowledgement leaves on its own port. A fourth port carries an operation
outside the closed set, refused there.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| request | MapRequest | One swap, publish or rollback, distinguished by `operation` and qualified by `map_generation` | implemented |
| lookup | RenameLookup | Read one name's current mapping; mutates nothing | implemented |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| map_swap_ack | MapRequest | New mapping installed. `reported_version` is the displaced version; `history_depth` is the new stack depth; `history_full` reports a refused swap | implemented |
| map_publish_ack | MapRequest | Version made architectural, or refused when the generation did not match the row | implemented |
| map_rollback_ack | MapRequest | Prior mapping restored. `logical_id` names the restored name, since a rollback does not choose one | implemented |
| refused | MapRequest | An operation outside the closed set, answered rather than routed out of range | implemented |
| answered | RenameLookup | Current version, last published version, and whether the current one is published | implemented |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- A map of `LOGICAL_TILE_REGS` rows, one per logical name, with exactly one
  writer. Instantiated per PE for Local and once per core for Shared
- Per row: current version, map generation, last published version, valid, and
  published
- A history stack of `MAP_HISTORY_DEPTH` entries holding displaced rows, plus one
  scalar stack depth
- No payload bytes, no descriptor, no physical slot, and no checkpoint records

## Reference implementation

[`rat.py`](rat.py) defines the mutating rule `serve_map_request`, the read-only
rule `answer_rename_lookup`, and the system `trn_rat_system`. Types and frozen
constants are shared through
[`contracts/tmu_trn.py`](../../contracts/tmu_trn.py).

The two storages are shaped differently on purpose. The map is *indexed* by
logical name, because a name is an index: the namespace is dense and every name
exists from reset. The history is a *stack*, because rollback must proceed
youngest-first and `ac.find`'s key selects the minimum key, so age cannot be
expressed as a search at all.

One rule serves all three operations, which is what makes the map single-port: a
rule admits one token per tick. Each owner has exactly one writer, as Decision
0151 requires. The lookup is a second rule reading the same map, so it never
queues behind a mutation.

The lowered module joins the swap's and the publish's map writes, since both
target the addressed row and the two operations are mutually exclusive. The
property that has to survive that join is the pairing: the history push is
guarded by exactly the swap half of the joined condition, which is what
`test_a_swap_installs_and_records_together` asserts.

## Departures from the original proposal

**The three mutating operations share one stream instead of three request ports.**
A rule fires only when *every* input has a token (Decision 0167/0168), and a swap,
a publish and a rollback arrive independently, so three ports would deadlock
waiting for each other. One stream also matches the hardware, since a single-port
map serves one operation per cycle. Acknowledgements still leave on separate
ports, via a plain route keyed on `operation`.

**The reset image is all-zero, so a reset name has no version rather than a
version.** This card asked for a nonzero architectural reset image. Decision 0151
admits an all-zero image only, so the row stores `valid` and a reset name reads as
"no version yet". A profile that needs identity mapping at reset -- name *i* maps
to version *i* -- cannot express it as an image; it needs either a nonzero-image
capability or an explicit initialization pass, and neither exists. This is the
same treatment FRE gives `allocated` rather than `free`.

**A fourth acknowledgement port exists for an operation outside the closed set.**
Three operations do not fill a two-bit branch, and a route selector outside
`[0, outputs)` is a deterministic runtime failure. A malformed request therefore
gets a port and a refusal rather than failing the model, because a request stream
must never produce "no answer".

**Local and Shared are separate instantiations of one system.** Shared cannot sit
inside the per-PE instance: it is core-owned, and four per-PE rules writing one
Shared map would give it four write endpoints, which Decision 0151 forbids.
`scope` is carried for the parent, which uses it to select an instance; RAT never
reads it, for the same reason BANK never reads `cell_key`.

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

This card listed namespace sizes, reset map, speculative-history depth and the
Shared namespace encoding as open. The implementation freezes three of them:

| Decision | Value | Reason |
| --- | --- | --- |
| Logical namespace | `LOGICAL_TILE_REGS = 64` | A name is an index, so the namespace size is the map's shape |
| Name width | `ac.u6`, exactly the namespace | A name outside the namespace is unrepresentable rather than wrapped onto a live mapping |
| History depth | `MAP_HISTORY_DEPTH = 32` | Bounds speculation depth, not live versions: an entry is popped by the rollback or dropped by the publish that resolves it |
| Reset image | All-zero; a row stores `valid` | Decision 0151 admits an all-zero image only |
| History ordering | A stack with a scalar depth | `ac.find` selects a minimum key, so youngest-first is not expressible as a search |

The Shared `namespace_scope` encoding remains open; `scope` is carried and routed
by the parent, never interpreted here.

## Backend boundary

Persistent indexed variables are provisional state under Decision 0151, which
PYC/RTL must reject at an explicit boundary, so gfsim is the only backend that can
execute this leaf. [bank.md](../trf/bank.md) records the same boundary and the
framework work that would remove it.

## Required capabilities to verify

- Multidimensional parameterized map storage — **available in one dimension**; the
  map is a persistent indexed variable of flat structs. Local/PE projection is
  instance geometry, not a second dimension
- Associative speculative history Table — **available, and deliberately not
  associative**; the history is a stack because youngest-first cannot be a search
- Atomic map/history updates with multiple response paths — **available**; both
  writes commit under one condition, and the responses fan out through a route
- Nonzero architectural reset image — **not available**; see "Departures"
- PYC/RTL state lowering — **rejected by design** while the storage is provisional
  state; see "Backend boundary"

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Four PE flows with identical logical IDs remain separate — **holds
  structurally**; the map is declared inside the system, so each instance owns a
  separate map and no port exposes one
- Shared has one map owner — **holds**; Shared is one instantiation with one
  writing rule, rather than a core table written by four per-PE rules
- Repeated RenameTxnKey returns prior acknowledgement — **partially holds**;
  idempotence here is generation-qualified rather than transaction-qualified. A
  repeated publish or rollback whose generation no longer matches is refused and
  changes nothing, which is the safety property. A repeated *swap* installs a
  second version, because RAT has no reservation row to recognize it by --
  [FRE](fre.md) owns `txn_key` idempotence, and the coordinator that holds both
  must not re-issue a swap it already got an acknowledgement for
- Full history stalls before FRE allocation commits — **not met**; see the first
  open decision. RAT refuses the swap with `history_full` and leaves the stack
  untouched, so nothing is corrupted and the swap can be retried, but the request
  is consumed rather than held
- Rollback restores only matching history generation and proceeds youngest-first —
  **holds**; the stack top must match `map_generation`, and popping a stack is
  youngest-first by construction

Design-local evidence: `designs/davincioo/tests/fabric/test_rat_alias_table.py`.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- **A full history cannot stall its requester, so that acceptance is unmet.**
  Decision 0210 fixes a rule's consumption condition to a proven-constant-true
  candidate and states that the false path **may consume input**, so a condition
  derived from history depth cannot hold the request in its queue. RAT therefore
  always answers, with `history_full` set. Nothing is lost, but backpressure
  becomes retry. This is the same gap FRE and REF hit, not a third one; closing it
  needs state-qualified consumption in the framework, which belongs upstream. The
  design-local test pins current behavior, so the assertion fails once the
  capability lands.
- **A reset map cannot be an identity mapping.** Only an all-zero image exists, so
  "name *i* maps to version *i* at reset" is not expressible. Whether the
  architecture actually requires it, or whether "no version yet" is the correct
  reset semantics, must be decided before this is called a gap.
- **Swap idempotence has no owner inside RAT.** `txn_key` rides the request and is
  never compared, because recognizing a repeat needs a reservation row and that row
  lives in FRE. Whether RAT should keep its own transaction row, or the coordinator
  must guarantee at-most-once delivery, is unresolved.
- **A name width is a literal, not a derived expression.** The frontend requires a
  static width in the annotation because it parses annotations as source text, so
  `LOGICAL_INDEX_BITS` cannot size the field. An assertion keeps the literal and
  the namespace consistent, but changing the namespace requires editing both.
- **The Shared instance has no confirmed host**, for the same reason FRE's Shared
  pool does not: whether the parent is TRN assembly or a core-level assembly is
  not recorded.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [ ] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [ ] Decide whether swap idempotence is owned here or guaranteed by the coordinator.
- [ ] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [ ] Implement the accepted owner and design-local expected-result tests.
- [ ] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:195` — Detailed RAT packet.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:613` — CoreSharedArray has one RAT/FRE/STS owner.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
