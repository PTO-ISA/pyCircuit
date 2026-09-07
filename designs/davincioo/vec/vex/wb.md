# VEC.VEX.WB — Writeback

- Source candidate: `DAV-VEC-VEX-WB-0001`
- Hardware hierarchy: **H3**, within H1 `VEC` / H2 `VEX`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **alias** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

alias/control in DBUF. Publication is outside VEC; do not create a second writer.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| operand_or_control | TBD | operand or control | unresolved |
| cancel_in | EpochCancel | cancel in | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| result_or_status | TBD | result or status | unresolved |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- No independent owner; realize within or at the reviewed parent boundary: Publication is outside VEC; do not create a second writer.

## Required capabilities to verify

- Exact packed scalar record widths and Queue backpressure; do not introduce or restore a PYC SIMD type.
- Owner-local bounded Tables/Arrays and generation-qualified reset/flush behavior where state exists.
- Carry dtype, numeric_control, valid region, definedness and exception metadata explicitly to a PTO-authoritative numeric oracle.

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Compile a typed Queue-only boundary through ACIR verifier, QueueGraph, C++ and Verilog with no residual host object semantics.
- Directed backpressure test proves no loss, duplication, partial state update or identity substitution.
- Compare representative and boundary numeric cases against the PTO/ASL oracle for each admitted dtype, rounding and exception mode.
- Demonstrate that this candidate introduces no second mutable owner or independently ticking module.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Which exact dtype, accumulator width, rounding, saturation and exception profile is admitted for the first specialization?
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

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:7527` — Generated catalog identity, hierarchy and planned source path.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:133` — Design review disposition: alias/control in DBUF; Publication is outside VEC; do not create a second writer.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:157` — VEC H1/H2/H3 inventory and state-owner design direction.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:67` — Common OpKey embeds exact FlowKey and operation generation.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:15` — PTO ASL remains authoritative for dtype, numeric results, rounding, exceptions and fault ordering.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
