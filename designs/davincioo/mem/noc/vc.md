# MEM.NOC.VC — Virtual Channel

- Source candidate: `DAV-MEM-NOC-VC-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `NOC`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **review** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/noc/vc.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

VC allocation/credit/release is a real state owner only if the selected NOC profile uses VCs; topology and ownership are not frozen.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| vc_alloc_req | VcAllocReq | Route/output-qualified VC request | unresolved |
| credit_return | VcCredit | Returned capacity for exact VC generation | unresolved |
| vc_release_req | VcReleaseReq | Release after terminal flit/packet transfer | unresolved |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| vc_alloc_ack | VcAllocAck | VC/generation ownership grant | unresolved |
| vc_release_ack | VcReleaseAck | Release result | unresolved |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- VC owner/generation
- Credits or Queue-capacity projection
- Release-pending state

## Required capabilities to verify

- Atomic allocator and credit accounting
- Queue-capacity versus explicit-credit invariant
- Generation-safe release

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- No double-counting Queue capacity and explicit credits
- VC reuse is generation safe
- Backpressure retains ownership until accepted release

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Decide whether first profile has VCs/explicit credits and whether state belongs inside RTR.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:144` — VC candidate is leaf/state owner.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:810` — Queue capacity is baseline credit; separate credit traffic needs a reviewed profile.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
