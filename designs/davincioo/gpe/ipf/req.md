# GPE.IPF.REQ — Request

- Source candidate: `DAV-GPE-IPF-REQ-0001`
- Hardware hierarchy: **H3**, within H1 `GPE` / H2 `IPF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **interface** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

input Queue schema. Packet type, not a module.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| packet_in | GPEPacket | packet in | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| packet_out | GPEPacket | packet out | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- No independent owner; realize within or at the reviewed parent boundary: Packet type, not a module.

## Required capabilities to verify

- Exact packed scalar record widths and Queue backpressure; do not introduce or restore a PYC SIMD type.
- Owner-local bounded Tables/Arrays and generation-qualified reset/flush behavior where state exists.
- Support exact GroupKey/FlowKey records, four independent PE Queue sets, participant masks and inferred atomic rendezvous/commit groups.

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Compile a typed Queue-only boundary through ACIR verifier, QueueGraph, C++ and Verilog with no residual host object semantics.
- Directed backpressure test proves no loss, duplication, partial state update or identity substitution.
- Four-flow SPMD test uses equal local BID/PC where useful and proves only explicit GroupKey plus participant generation completes rendezvous.
- Demonstrate that this candidate introduces no second mutable owner or independently ticking module.

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

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:9749` — Generated catalog identity, hierarchy and planned source path.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:197` — Design review disposition: input Queue schema; Packet type, not a module.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:160` — GPE H1/H2/H3 inventory and state-owner design direction.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:67` — Common OpKey embeds exact FlowKey and operation generation.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:21` — Selected profile is four independent PE control flows under one SPMD ELF/entry.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
