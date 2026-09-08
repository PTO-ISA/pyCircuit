# SPE.OOO.ROB — Reorder Buffer

- Source candidate: `DAV-SPE-OOO-ROB-0001`
- Hardware hierarchy: **H3**, within H1 `SPE` / H2 `OOO`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **independent H3 leaf**
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/spe/ooo/rob.py`
- Current design-program execution status: **implemented and gfsim behavior verified;
  NDF traceability review, H2/H1 integration and PYC/RTL remain pending**.

The frozen catalog records external provenance. The reviewed local contract and
current execution evidence are below.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| flush_request | RobEvent | complete recovery transaction | declared |
| allocate_request | RobEvent | tail-row reservation | declared |
| completion | RobCompletion | generation-qualified writeback result and exception | reviewed 2026-09-08 |
| handoff_ack | RobHandoff | durable CMT handoff | declared |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| allocated | RobEvent | allocated RobKey | declared |
| committed | RobEvent | oldest completed MicroCommit | declared |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- per-flow circular ROB rows with slot generation and completion/handoff state

## Required capabilities to verify

- typed compound Queue payloads
- atomic input/state/output rule semantics
- bounded Table/Array lowering with generation-qualified identity

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- All declared/proposed Queue outputs remain stable under backpressure and preserve full identity/generation.
- Stale, duplicate, wrong-flow, and post-recovery responses cause no mutation.
- gfsim and generated C++/Verilog agree at accepted Queue transfers.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- No additional item recorded; exact state/port review remains required.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [x] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [x] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [x] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [x] Implement the accepted owner and design-local expected-result tests.
- [x] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:1` — Catalog row: OOO.ROB, source srcs/core/spe/ooo/rob.py, status planned/deferred.
- `docs/specification/davincioo/ndf-next/scalar/ooo.md:145` — Normative NDF owner/refinement clause.
- `docs/architecture/core/l3/SPE_EXECUTION_PACKETS.md:396` — Detailed proposed module card or disposition evidence.
- `docs/specification/davincioo/ndf-next/modules/spe/ooo/rob.md:42` — Typed Queue/state contract for existing source draft.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.

## Reviewed implementation contract (2026-09-08)

The maintainer-approved ROB plan freezes this design-local interface under
Decision 0222. Decisions 0173–0177, 0189, 0194, 0196, 0201, 0224 and 0226 govern
inferred arbitration, atomic state, host backpressure and nominal identity.
Decision 0228 governs recording. This card does not promote unreviewed NDF links.

Each `rob` module owns one flow and sixteen slots, matching `RobKey.slot: u4`.
The structural `core_id`, `pe_id`, `stid`, and `launch_generation` parameters bind
its FlowKey; their respective legal widths are 4, 2, 4 and 16 bits. Each instance
owns its entries, head, tail, five-bit count, sixteen-bit recovery epoch,
pending-handoff flag and recovery-wait flag. No CMT or W2 state is duplicated.

`allocate_request: RobEvent` accepts valid ALLOCATE messages whose four flow
identities match the instance and whose epoch is current. Allocation increments
the selected slot generation, initializes completion fields, advances tail and
count, and publishes ALLOCATED atomically. Full capacity, recovery wait or a full
allocation output retains the request. Invalid allocation requests also remain
at the boundary: the upstream producer must supply a well-formed current request
and drain stale requests during recovery before submitting a replacement.

`completion: RobCompletion` comes from the writeback/apply seam. The addressed
entry must be live in the current epoch, incomplete and not handed off, and its
EpochKey, InstKey, BlockKey and RobKey must all match. Issue-attempt retry metadata
is not allocated by ROB and is owned/validated by WBA/W2. ROB takes the first
identity-matching completion; later duplicates are consumed without mutation.
Result, result-valid, TerminalStatus, 32-bit fault code, 64-bit fault argument,
fault BI and fault-valid are preserved exactly in the microcommit message.

A completed head publishes MICROCOMMIT to `committed` atomically with setting
pending-handoff. Publication to that Queue is the irreversible handoff boundary,
including when the CMT consumer has not yet popped the token. Count and head do
not change. At most one entry per instance awaits acknowledgement.

`handoff_ack: RobHandoff` releases only that pending entry, matching all four
identity records, valid and durable, the original required-owner mask, all
required received-owner bits, and exact required/acknowledged history counts.
CMT must count distinct durable histories and prevent duplicated acknowledgements
from fabricating that count. ROB does not independently reconstruct CMT history.
Duplicate, malformed and stale acknowledgements are consumed without release.

A valid FLUSH names the current source epoch. It advances epoch once and
logically invalidates every unhanded entry. Physical row images remain available
for generation retention and diagnostics; validity requires the current epoch,
except for the explicitly pending handoff. If pending, count becomes one and
allocation/handoff wait for its old-epoch durable ack. Otherwise count becomes
zero and head moves to tail. On the retained ack, head moves to the saved tail
and recovery ends. Old flushes and requests received while waiting are consumed
without advancing epoch. Reusing a source epoch for a different recovery is an
upstream protocol error.

Lexical conflict priority is flush, acknowledgement, completion, handoff, then
allocation. Rules observe one committed snapshot. Flush conflicts with new
allocation/completion/handoff through epoch/count/recovery state; its pending
ack may retry at the next boundary and remains serviceable throughout recovery.
Nonconflicting transactions may commit together. There is no same-cycle
completion-to-handoff bypass. All selected outputs participate in their rule's
atomic commit; backpressure leaves that rule's state unchanged.

Reset clears all scalar state and all sixteen row images to zero before recording
starts. The first allocation uses generation one. Slot generations and recovery
epochs wrap modulo 65536: the environment must not retain responses across
65536 reuses of one slot or 65536 recoveries. Reset also requires the environment
to drain pre-reset responses; reset does not preserve anti-ABA history.

NDF provenance remains the frozen external sources listed above. The supplied
plan confirms this L2 behavior; specific L0/L1 clause correspondence remains
**pending review**, and does not imply whole-OOO or architectural validation.
CMT/W2 implementation, H2/H1 integration and PYC/RTL are outside this delivery.

## Reproduction and evidence

Run through `/home/lc/.codex/skills/pyc6/scripts/run.sh`:

```text
python -m pytest -q designs/davincioo/tests/spe/ooo/test_rob.py
```

Set `PYC_DAVINCIOO_ROB_OUT` to select the artifact directory. The default is
`.pycircuit_out/davincioo-rob/20260908-rob/`. It retains generated/raw/frozen
sources and a compiled driver, with per-scenario independent committed-state
projections, scan and activation traces, stdout and offline HTML. Tests first
check expected behavior without recording, then repeat identical inputs with
recording and compare the full per-boundary projection and scheduler counters.
Reviewable gate results and remaining gaps are recorded in
[the gate summary](../../../../docs/gates/logs/20260908-davincioo-rob/summary.md).

### Single-instance replay

`tests/spe/ooo/test_rob_single.py` selects `rob_system` and generates one ROB
instance with four inputs, two outputs and five rule operations. Its capacity,
backpressure, invalid-response and recovery scenarios use the shared independent
expected-result driver and compare complete scan/activation and recording
projections. Reset completes before recording.

From the repository root in the pyc6 environment:

```sh
python -m pytest designs/davincioo/tests/spe/ooo/test_rob_single.py -q
```

The default replay index is
`.pycircuit_out/davincioo-rob/20260908-single/index.html`;
`PYC_DAVINCIOO_SINGLE_ROB_OUT` selects another output directory.
Evidence: `docs/gates/logs/20260908-single-rob/summary.md`.

### Rule-level NDF sample

[Allocation rule NDF draft](ndf/rob/allocate.md) demonstrates one rule definition
per format-0.2 document, with stable clause identities and local verification
relationships. It does not close the pending L0/L1 traceability review.
