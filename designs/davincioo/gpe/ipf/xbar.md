# GPE.IPF.XBAR — Crossbar

- Source candidate: `DAV-GPE-IPF-XBAR-0001`
- Hardware hierarchy: **H3**, within H1 `GPE` / H2 `IPF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/gpe/ipf/xbar.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

registered transport leaf. Moves one retained packet per accepted grant.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| ingress_0 | GPEPacket | ingress 0 | declared |
| ingress_1 | GPEPacket | ingress 1 | declared |
| ingress_2 | GPEPacket | ingress 2 | declared |
| ingress_3 | GPEPacket | ingress 3 | declared |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| delivered | GPEPacket | delivered | declared |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Round-robin ingress merge, destination routing and retained per-destination transport queues; no PTO execution state.

## Required capabilities to verify

- Existing Queue merge/route/apply primitives and exact scalar-packed GPEPacket support.

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Four declared ingress Queues preserve FIFO order per source and route a complete packet to its destination under round-robin arbitration.
- Backpressure retains complete GPEPacket identity; completed/delivered_pe mean transport delivery only.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- The registered transport leaf does not close GGC rendezvous, group completion or PTO instruction execution.

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

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:9860` — Generated catalog identity, hierarchy and planned source path.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:196` — Design review disposition: registered transport leaf; Moves one retained packet per accepted grant.
- `docs/specification/davincioo/ndf-next/tile/gpe.md:62` — Normative candidate clause (draft status).
- `srcs/core/gpe/ipf/xbar.py:7` — Declared GPEPacket fields and exact ac.Queue module signature.
- `srcs/core/gpe/ipf/xbar.py:25` — Four typed ingress Queues, round-robin merge, four-way route and one delivered Queue.
- `docs/specification/davincioo/ndf-next/interfaces/modules.md:75` — Normative GPE IPF ingress boundary and full-packet identity retention.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:160` — GPE H1/H2/H3 inventory and state-owner design direction.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:67` — Common OpKey embeds exact FlowKey and operation generation.
- `docs/architecture/core/l3/COMPUTE_GROUP_PACKETS.md:21` — Selected profile is four independent PE control flows under one SPMD ELF/entry.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
