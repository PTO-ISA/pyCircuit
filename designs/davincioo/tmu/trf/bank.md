# TMU.TRF.BANK — Register Bank

- Source candidate: `DAV-TMU-TRF-BANK-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trf/bank.py`
- Current design-program execution status: **source implementation present at the
  correct whole-cell granularity; gfsim verification pending**. Storage is a
  persistent indexed variable, which Decision 0151 classifies as provisional
  state, so PYC and RTL must reject this leaf at an explicit boundary. See
  "Backend boundary".

BANK is where a Tile's data bytes actually live: one physical bank, one whole
128-byte cell per row, plus a generation that says which version a row holds.

## What problem it solves

The bytes a Tile instruction writes have to land somewhere physical. BANK is that
somewhere -- a single-port bank whose every entry holds one whole 128-byte cell.

BANK does not know whose bytes those are or what they mean. It does two things
only: read or write an entire cell by row number, and use a generation to decide
whether the row currently holds the version the requester asked for. Three access
kinds cover all of it: read, write, and invalidation.

Because the storage granularity is the access granularity, no address arithmetic
can reach a fragment of a cell, and there is no sub-cell write mask that could be
applied to the wrong bytes.

## Where it sits

A Tile's lifetime is divided among several modules. BANK owns exactly one part of
it -- the bytes themselves:

| Module | Owns | Does not own |
| --- | --- | --- |
| [RAT](../trn/rat.md) | Logical name to version mapping | Where that version's data lives |
| [FRE](../trn/fre.md) | Which physical block is free, and which block a version holds | What is in the block |
| [STS](../trn/sts.md) | A version's descriptor, definedness and publication status | The data bytes |
| **BANK** | **The data bytes themselves** | **Who owns the block, or when it may be reclaimed** |
| [REF](ref.md) | How many readers still hold a version | Whether reclaim is actually permitted |
| [LTR](../trn/ltr.md) | Sequencing the siblings above into one ordered transaction | Any of their state |

An acknowledgement from BANK therefore means bytes changed, never that a Tile
became visible. Publication and definedness stay in STS/RAT, and reclaim
permission in FRE/REF.

On the fabric side, [MAP](../bgf/map.md) resolves a `cell_key` into a
`(bank, row)` pair, [ARB](../bgf/arb.md) decides which source class reaches a bank
this tick, and [RQ](../bgf/rq.md) / [WQ](../bgf/wq.md) only queue. BANK receives an
access that is already located and already granted, so it never reads `cell_key`
and never arbitrates.

Two neighboring candidates describe state rather than modules. [cell.md](cell.md)
is the schema of what one entry holds, and it lives in the two arrays this leaf
elaborates. [rdy.md](rdy.md) is a projection derived from STS coverage and BANK
acknowledgements, so nothing here stores readiness.

## How one access completes

All three access kinds arrive on one stream and are told apart by `kind`.

The request carries `row` and `tile_version`. `row` is masked first, so an
out-of-range row cannot reach another cell's payload; the bound is structural
rather than a promise from upstream. BANK then reads the generation of that row
and the cell the row currently holds.

**The generation comparison is the whole permission check.** A write commits only
if the stored generation equals the requested `tile_version`. A stale write is
demoted to a read of the same row, because an old generation must not alter a cell
that has since been reused. The request still produces a response either way,
since the requester is waiting for one. `current` reports the generation match and
`applied` reports whether bytes changed, so a rejected write is observable rather
than silent.

**An invalidation installs a new generation and touches no payload bytes.**
Clearing the cell would be wasted work: a generation mismatch already makes the
old bytes unreachable, and no reader can observe them afterwards.

**The record that comes back carries the cell as committed before this tick.** A
write therefore returns the cell it replaced, and that is exactly what
distinguishes a demoted stale write from one that took effect.

The response leaves two cycles after the access arrives: one cycle from the rule
that touches the arrays, one from the output stage.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| access | BankAccess | One granted access of one bank: read, write, or invalidation, qualified by `tile_version` and located by `row`, which is the only coordinate | implemented seam |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| completed | BankAccess | Outcome of any of the three kinds. `word0` .. `word15` carry the cell as committed before this tick, `current` reports the generation match, `applied` reports whether bytes changed, and `stored_generation` carries the generation that was read or replaced | implemented seam |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Cell array of `ROWS_PER_BANK` entries, one whole 128-byte cell per entry,
  indexed by `row`
- CellGeneration per cell, in a separate array indexed by the same `row`
- No descriptor, definedness or publication state, and no identity: `CellData`
  holds payload words only

## Reference implementation

[`bank.py`](bank.py) defines the rule `serve_bank_access` and the system
`trf_bank_system`, elaborated once per bank; `BankAccess`, `CellData` and the
geometry constants are shared through
[`contracts/tmu_trf.py`](../../contracts/tmu_trf.py).

Two arrays are elaborated, both indexed by `row` alone. **One entry of the cell
array is one whole 128-byte cell**, and the lowered layout spec confirms the
element size is exactly 128 bytes. The generation array holds one generation per
cell.

One rule touches both arrays, and that is what makes the bank single-port: a rule
admits one token per tick, so at most one access reaches the arrays per cycle.
Each array has exactly one writer, which Decision 0151 requires anyway.

The generation check qualifies the write in the same tick, so no separate decision
stage is needed. The returned cell is the pre-tick one because Decision 0151 makes
a proposed write visible only at tick commit, so reading the array after the
conditional write still observes the replaced value.

Masking `row` matters because the `out_of_range` result [map.md](../bgf/map.md)
computes is reported and never enforced. Decision 0151 additionally reports
`table_index_out_of_range` at runtime, but only for indices the mask cannot
produce.

Identity never enters storage. `CellData` holds payload words and nothing else,
deliberately: a cell outlives the request that wrote it, so storing identity would
make a later read return the *previous writer's* identity and misroute the
response. Response identity comes from the request record.

## Departures from the original proposal

**One record covers all three access kinds** rather than the three request and
three response types this card originally proposed. All three contend for the same
arrays, so splitting them would create two ways into one physical port. It also
matches ARB, which already merges reads and writes into one granted stream per
bank, so a single-port bank sees one access at a time by construction.

**Storage holds one whole cell per entry, not one word per entry.** The earlier
word-granular revision was expressible on `ac.memory` and lowerable to RTL, but it
exposed a sub-cell granularity clients never request and spent 4 cycles on a read
instead of 2. Correct granularity was chosen at the price of a backend; the
trade-off is recorded in full under "Backend boundary".

## Frozen geometry

| Parameter | Value | Note |
| --- | --- | --- |
| `ROWS_PER_BANK` | 256 | One row is one entry and holds one whole cell |
| `CELL_BYTES` | 128 | Confirmed as the lowered element size |
| `WORDS_PER_CELL` | 16 | Representation only: sixteen `u64` fields of one indivisible payload, forced by the 64-bit integer limit. Not a coordinate; nothing selects among them |
| `CELL_READ_LATENCY` | 2 | **End-to-end at this leaf's output**, not one internal stage |
| `RULE_ACCESS_LATENCY` | 1 | Contributed by the rule that touches the arrays |
| `CELL_OUTPUT_STAGES` | 1 | Supplies the remainder of the read latency |
| Accesses per cycle | 1 | Single-port, because one rule admits one token per tick |

Read latency is exactly 2 cycles as required. The earlier word-granular revision
spent 4, because a generation read and a separate decision stage each added one.

## Backend boundary

Persistent indexed variables are **provisional state** under Decision 0151, and
PYC/RTL must reject them at an explicit boundary. gfsim is therefore the only
backend that can execute this leaf until Decision 0151 is superseded. This card's
own gate policy anticipates exactly this case: gfsim execution is the first
implementation gate, and PYC/RTL obligations remain explicit future work where
provisional storage is rejected.

That boundary is the price of correct granularity, and it was chosen knowingly.
Four forms are expressible or nearly so, and only the last is free of both costs:

| Form | Storage | Access slots per cell | Read latency | Backends | Verdict |
| --- | --- | --- | --- | --- | --- |
| One entry per cell | Persistent indexed variable | 1 | 2 | gfsim only | **Current.** Architecturally correct; PYC/RTL rejected |
| Word-granular | `ac.memory` | 1 per word | 4 | gfsim, PYC, RTL | Superseded. Exposed a sub-cell granularity clients never request |
| 16 chained endpoints | `ac.memory` | 16 | 32 | gfsim, PYC, RTL | Rejected. Fabricates serial cost the hardware does not spend |
| One entry per cell | `ac.memory` with an aggregate data type | 1 | 2 | gfsim, PYC, RTL | Target end state; needs the framework fix below |

The last row remains worth pursuing because it is the only form that is both
architecturally correct and lowerable to RTL. It requires admitting aggregate
memory data types, which touches three layers. The reproducer is
`tests/python/agentic-circuit/python_frontend/test_queue_frontend.py::
test_memory_rejects_aggregate_data_type_with_a_source_diagnostic`.

| Layer | Location | State |
| --- | --- | --- |
| Frontend type resolution | `_queue_frontend.py:1240` (`_payload`) | Rejects `ac.array[...]`; diagnostic misleadingly says `source payload` |
| Dialect verifier | `compiler/acir/lib/Dialect/ACIR/ACIROps.cpp:2645` | `data type must be an integer no wider than 64 bits` |
| gfsim runtime | `simulator/gfsim/include/gfsim/queue_blocks.h` (`QueueMemory`) | Likely already generic: `Data` is a template parameter and `storage_` is `std::vector<Data>` |

This is a reusable framework capability, not a DavinciOO-local fix, so per
AGENTS.md it belongs upstream in `PTO-ISA/pyCircuit` rather than in this fork.
It also extends a documented type contract, so it needs a decision update rather
than a silent verifier relaxation. Local verification of the dialect and gfsim
layers was not possible in this checkout: the build requires MLIR 22.1.8 exactly
and no LLVM/MLIR toolchain is installed.

## Required capabilities to verify

- Banked Array with masked writes — **not required**; access granularity is one
  whole cell, so no sub-cell mask exists
- Banked Array whose entry is one whole 128-byte cell — **available** via a
  persistent indexed variable whose element is a flat struct (Decision 0151)
- Atomic Queue consume plus Array update plus ack — available; one rule updates
  both arrays and returns the response atomically
- Multiple independent input/output Queues — available
- Parameterized read-latency pipeline — available; the rule stage plus an output
  stage sum to the frozen latency
- PYC/RTL lowering for stateful Array hierarchy — **rejected by design** while
  storage is provisional state; see "Backend boundary"

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Baseline services at most one access per cycle because selected banks are
  single-port -- **holds**; one rule admits one token per tick, so no second access
  to these arrays can exist in the same cycle
- A blocked response/pipeline slot prevents read grant -- **holds structurally**
  for BANK's share of it. A rule fires only when its output has capacity
  (Decision 0167/0168), so a blocked response slot stops BANK from consuming the
  request and from reading at all. The word "grant" belongs to ARB; what BANK
  guarantees is that it does not consume
- A write replaces one whole cell; no sub-cell write exists to be checked --
  **holds structurally**; one entry is one whole cell, so a partial write has no
  representation here to check
- Old CellKey generation cannot alter a reused row -- **holds**; a write requires a
  generation match, and a stale write is demoted to a read of the same row
- An out-of-range row cannot reach another cell's payload -- **not met**. Masking
  bounds the index but aliases rather than refuses: a row past ROWS_PER_BANK wraps
  onto a different valid row of this bank, which is exactly another cell's payload.
  What masking does guarantee is that the access stays inside this bank and cannot
  fault. See the second open decision
- BANK never changes descriptor/definedness or publishes Tile state -- **holds
  structurally**; no rule touches those fields and no port carries them

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- **What a stale read means to the caller.** BANK reports `current` and still
  returns bytes; the rejection policy owner is unfrozen, as it is for the
  `out_of_range` result in [map.md](../bgf/map.md).
- **An out-of-range row aliases instead of being refused, and refusal has no
  owner.** MAP computes `out_of_range` and never enforces it, BANK masks the row so
  the access always lands somewhere legal, and nothing in between rejects it. The
  consequence is that a malformed `cell_key` reads or writes a different cell of the
  same bank rather than failing. Masking is still the right structural bound, since
  it keeps the access inside this bank and keeps Decision 0151's
  `table_index_out_of_range` from firing, but the enforcement point has to be frozen
  somewhere and this leaf cannot be it: refusing an access needs a response that
  says "refused", which changes the request record shared with MAP, RQ/WQ and ARB.
- **Generation zero.** The generation array initialises to 0 and Decision 0151
  supports only an all-zero initial image, so a request carrying
  `tile_version == 0` matches an untouched row. Either `tile_version` starts at 1
  by upstream convention, or invalidation must run before first use; neither is
  recorded yet.
- **Whether to pursue the aggregate-memory framework fix.** It is the only form
  that is both architecturally correct and lowerable to RTL. Until then this leaf
  is gfsim-only, which is sufficient for the first implementation gate but not for
  an RTL obligation.
- Freeze Queue depths if any profile needs more than the minimum. Row count,
  read latency and the one-access-per-bank baseline are frozen above.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:277` — Detailed BANK packet and raw payload ownership.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:738` — Selected proposal derives from 32 single-port 128-byte banks; first profile uses private groups.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:789` — Descriptor fields explicitly excluded from raw SRAM cell ownership.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
