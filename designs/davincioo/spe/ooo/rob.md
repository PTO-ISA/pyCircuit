# SPE.OOO.ROB — Reorder Buffer

- Source candidate: `DAV-SPE-OOO-ROB-0001`
- Hardware hierarchy: **H3**, within H1 `SPE` / H2 `OOO`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/spe/ooo/rob.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

A typed NDF Queue contract exists and may have a Python design draft, but the catalog still says planned/deferred; promote only after behavioral and backend gates.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| flush_request | RobEvent | complete recovery transaction | declared |
| allocate_request | RobEvent | tail-row reservation | declared |
| completion | RobEvent | generation-qualified completion | declared |
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
- [ ] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [ ] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [ ] Implement the accepted owner and design-local expected-result tests.
- [ ] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:1` — Catalog row: OOO.ROB, source srcs/core/spe/ooo/rob.py, status planned/deferred.
- `docs/specification/davincioo/ndf-next/scalar/ooo.md:145` — Normative NDF owner/refinement clause.
- `docs/architecture/core/l3/SPE_EXECUTION_PACKETS.md:396` — Detailed proposed module card or disposition evidence.
- `docs/specification/davincioo/ndf-next/modules/spe/ooo/rob.md:42` — Typed Queue/state contract for existing source draft.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
