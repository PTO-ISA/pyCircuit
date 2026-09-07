# MEM.MIF.ORD — Ordering

- Source candidate: `DAV-MEM-MIF-ORD-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `MIF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/mif/ord.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Owns per-agent PTO ordering dependencies and visible memory-effect permits; cache/NOC completion cannot replace this boundary.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| order_reserve_req | OrderReserveReq | Install agent/order/address-range dependency row | proposed |
| effect_permit_req | EffectPermitReq | Request authorization for a visible effect | proposed |
| effect_complete_req | EffectCompleteReq | Retained terminal effect completion | proposed |
| fence_req | FenceReq | Agent-qualified fence operation | proposed |
| order_cancel_req | OrderCancelReq | Cancel row with no visible effect | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| order_reserve_ack | OrderReserveAck | Order row reservation result | proposed |
| effect_permit_ack | EffectPermitAck | BROB- and predecessor-qualified effect permit | proposed |
| effect_complete_ack | EffectCompleteAck | Frontier advancement result | proposed |
| fence_ack | FenceAck | Fence completion | proposed |
| order_cancel_ack | OrderCancelAck | Cancel result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- OrderTable per memory agent/order sequence
- Operation/address-range/acquire/release/fence dependencies
- BROB publication permit
- Effect-issued and terminal status
- Per-agent order frontiers

## Required capabilities to verify

- Associative/range dependency Table
- Multiple independent Queue pairs
- Atomic visible-effect issue plus effect_issued update
- Explicit predecessor selection/conflict rules
- PYC/RTL Table lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- MicroCommit alone never permits a visible store
- Permit requires exact PTO predecessors and BROB authorization
- Visible effect and effect_issued transition are one event
- Recovery removes only rows without visible effect
- Blocked acknowledgement cannot advance order frontier

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Catalog ID is 0001 while normative owner anchor is DAV-MEM-MIF-ORD-0002; resolve identity before registration.
- Freeze same-location dependency representation and order capacity.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:393` — Detailed ORD packet.
- `docs/specification/davincioo/ndf-next/tile/memory.md:33` — Normative ordering owner and event boundary.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
