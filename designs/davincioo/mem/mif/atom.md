# MEM.MIF.ATOM — Atomic Operation

- Source candidate: `DAV-MEM-MIF-ATOM-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `MIF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **review** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/mif/atom.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Atomic RMW sequencing is required under ORD, but leaf versus interface placement and exact PTO operation/effect contract are unresolved.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| atomic_req | AtomicReq | Typed operation, agent/order, address, operands, identity, and permit context | unresolved |
| effect_permit_ack | EffectPermitAck | ORD authorization for visible RMW effect | unresolved |
| atomic_mem_resp | AtomicMemResp | Lower-memory terminal data/fault | unresolved |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| effect_permit_req | EffectPermitReq | Request ORD-visible effect authorization | unresolved |
| atomic_mem_req | AtomicMemReq | Serialized lower-memory RMW transaction | unresolved |
| atomic_resp | AtomicResp | Typed old/new result or fault | unresolved |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Retained atomic transaction phase and operands are required

## Required capabilities to verify

- Compound atomic event across ORD and memory Queues
- Retained multi-cycle FSM
- Typed operation enum and arithmetic semantics
- PYC/RTL parity

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- No visible RMW before ORD permit
- Atomicity and PTO event ordering are preserved
- Fault/preflight cannot leave partial visible effect
- Generation-qualified retry completes once

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Select leaf ownership, supported atomic forms, lower-memory primitive, and exact effect boundary.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:135` — ATOM remains leaf/interface recommendation.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:918` — ORD and PTO memory ordering/atomic constraints.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
