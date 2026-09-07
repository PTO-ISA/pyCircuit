# MEM.MIF.GM — Global Memory

- Source candidate: `DAV-MEM-MIF-GM-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `MIF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **interface** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

External global-memory adapter boundary; it must not own internal cache/order state or freeze AXI/CHI topology.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| global_mem_req | GlobalMemReq | Bus-neutral qualified request from BHU/NOC | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| global_mem_resp | GlobalMemResp | Bus-neutral terminal data/ack/fault with original generation | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Any bus adaptation state belongs to the system integration implementation, not this semantic interface

## Required capabilities to verify

- Typed integration interface and adapter boundary
- Clock-domain support only if selected system requires it

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Preserve transaction/generation/agent/order/address/mask/return identity
- No implicit AXI or CHI semantics
- No cache-state or architectural-order ownership

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Define system adapter protocol and clock/reset domain outside the core semantic module catalog.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:136` — GM is an integration interface with no cache-state ownership.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:933` — External bus protocol remains an integration choice.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
