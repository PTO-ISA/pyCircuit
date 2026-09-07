# GPE.GMM.ISS — Issue

- Source candidate: `DAV-GPE-GMM-ISS-0001`
- Hardware hierarchy: **H3**, within H1 `GPE` / H2 `GMM`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/gpe/gmm/iss.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

leaf. Joins GGC wake, operands, and CUBE credit before issue.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| group_wake_in | GroupWake[pe] | group wake in | proposed |
| local_operand_ready_in | OperandReady[pe] | local operand ready in | proposed |
| shared_operand_ready_in | OperandReady[pe] | shared operand ready in | proposed |
| cube_credit_in | CubeCredit[pe] | cube credit in | proposed |
| cancel_in | EpochCancel | cancel in | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cube_issue_out | EngineCommand[pe] | cube issue out | proposed |
| group_issue_done_out | GroupIssueDone | group issue done out | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Issue[entries] with GroupKey, per-PE OpKeys, wake/operand/credit bits, issue mask, participant mask, generation and fault

## Required capabilities to verify

- Exact packed scalar record widths and Queue backpressure; do not introduce or restore a PYC SIMD type.
- Owner-local bounded Tables/Arrays and generation-qualified reset/flush behavior where state exists.
- Support exact GroupKey/FlowKey records, four independent PE Queue sets, participant masks and inferred atomic rendezvous/commit groups.

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Compile a typed Queue-only boundary through ACIR verifier, QueueGraph, C++ and Verilog with no residual host object semantics.
- Directed backpressure test proves no loss, duplication, partial state update or identity substitution.
- Four-flow SPMD test uses equal local BID/PC where useful and proves only explicit GroupKey plus participant generation completes rendezvous.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- What software/architectural rendezvous token and participant ordinal source freeze GroupKey for four independent PE control flows?
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

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:9441` — Generated catalog identity, hierarchy and planned source path.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:191` — Design review disposition: leaf; Joins GGC wake, operands, and CUBE credit before issue.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:539` — Detailed proposed state and Queue contract.
- `docs/specification/davincioo/ndf-next/tile/gpe.md:54` — Normative candidate clause (draft status).
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:160` — GPE H1/H2/H3 inventory and state-owner design direction.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:67` — Common OpKey embeds exact FlowKey and operation generation.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:21` — Selected profile is four independent PE control flows under one SPMD ELF/entry.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
