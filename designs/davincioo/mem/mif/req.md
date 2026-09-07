# MEM.MIF.REQ — Request

- Source candidate: `DAV-MEM-MIF-REQ-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `MIF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/mif/req.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Normalizes distinct ingress classes and owns request decomposition/preflight residency before TXN accepts fragments.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| request_ingress[] | ScalarReq\|TileReq\|IFillReq\|WritebackReq\|MaintenanceReq | Independent typed request-class Queues | proposed |
| translate_resp | TranslateResp | Generation-qualified translation/preflight result | proposed |
| txn_fragment_ack | TxnFragmentAck | TXN accepted exact fragment | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| translate_req | TranslateReq | Captured ACR/address-space/epoch translation request | proposed |
| txn_fragment_req | TxnFragmentReq | Qualified cacheline/lower-memory fragment | proposed |
| req_accept_ack | ReqAcceptAck | Complete request residency acknowledgement | proposed |
| req_fault | ReqFault | First retained preflight/decomposition fault | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- ReqTable keyed by MemTxnKey
- Request class/FlowKey/InstKey or CommandKey
- Agent/order/ACR/address-space/translation epoch
- Base/shape/stride/dtype/layout/masks/return target
- Fragment cursor/plan and first fault

## Required capabilities to verify

- Tagged-union/closed variant payloads
- Associative Table plus bounded vector/cursor
- Multi-input arbitration without cross-class head blocking
- Atomic admission/fragment issue
- PYC/RTL state lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Admission allocates a complete row or changes nothing
- One stalled class does not consume another
- Translation matches exact transaction generation
- Fragment cursor advances only when TXN accepts
- Packed-nibble stores retain preservation masks
- Zero-mask/no-effect emits no payload transaction
- Recovery retains externally visible/late-response obligations until ORD disposition

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze request-class schemas, translation owner/interface, fragment plan bound, per-agent quotas, and exact supported PTO forms.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:346` — Detailed REQ packet.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:841` — Separate memory ingress classes and common ordering owner.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
