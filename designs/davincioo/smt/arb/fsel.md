# SMT.ARB.FSEL — Fetch Selection

- Source candidate: `DAV-SMT-ARB-FSEL-0001`
- Hardware hierarchy: **H3**, within H1 `SMT` / H2 `ARB`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/smt/arb/fsel.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

leaf; detailed. Fair selection of eligible flow fetch requests

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| fetch_candidate | FetchCandidate[flow] | fetch candidate | proposed |
| eligibility_cancel | EligibilityCancel | eligibility cancel | proposed |
| grant_ack | FetchGrantAck | grant ack | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| fetch_grant | FetchGrant | fetch grant | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Eligible[FlowKey], independent rr_cursor and retained grant

## Required capabilities to verify

- Exact packed scalar record widths and Queue backpressure; do not introduce or restore a PYC SIMD type.
- Owner-local bounded Tables/Arrays and generation-qualified reset/flush behavior where state exists.
- Represent FlowKey(core_id, pe_id, stid, launch_generation) and recovery epoch explicitly on every cross-owner transaction.

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Compile a typed Queue-only boundary through ACIR verifier, QueueGraph, C++ and Verilog with no residual host object semantics.
- Directed backpressure test proves no loss, duplication, partial state update or identity substitution.
- Run two or four equal-STID flows with distinct PE/launch generations and prove isolation through stall, replay and recovery.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Exact payload field widths, Queue depth/rate/latency and parent-owned seam remain proposed unless stated by the cited detailed card.

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

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:4893` — Generated catalog identity, hierarchy and planned source path.
- `docs/architecture/core/l3/FRONTEND_CONTEXT_PACKETS.md:64` — Design review disposition: leaf; detailed; Fair selection of eligible flow fetch requests
- `docs/architecture/core/l3/FRONTEND_CONTEXT_PACKETS.md:167` — Detailed proposed state and Queue contract.
- `docs/specification/davincioo/ndf-next/scalar/smt.md:60` — Normative candidate clause (active status).
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:154` — SMT H1/H2/H3 inventory and state-owner design direction.
- `docs/architecture/core/l3/FRONTEND_CONTEXT_PACKETS.md:93` — Common FetchKey/FlowKey identity must survive all frontend and context transactions.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
