# TMU.BGF.XBAR — Crossbar

- Source candidate: `DAV-TMU-BGF-XBAR-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `BGF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/bgf/xbar.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Existing registered transport leaf; it routes complete BGFPacket transactions and marks destination delivery only.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| ingress_0 | BGFPacket | Independent requester ingress | declared |
| ingress_1 | BGFPacket | Independent requester ingress | declared |
| ingress_2 | BGFPacket | Independent requester ingress | declared |
| ingress_3 | BGFPacket | Independent requester ingress | declared |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| delivered | BGFPacket | Packet after selected destination-bank delivery; completed and delivered_bank are set | declared |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Internal merge, route, destination latency, and completion Queue stages only

## Required capabilities to verify

- Already supported merge/route/apply hierarchy
- Production replacement must connect grants and BANK acknowledgements rather than treating pilot latency as a bank

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- FIFO within each ingress and round-robin arbitration across ingresses
- Retain full packet under output backpressure
- Delivery marker is transport completion, never Tile publication or PTO commit

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Decide whether the existing pilot remains the production XBAR or is replaced by an ARB-grant-driven transport leaf.

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

- `srcs/core/tmu/bgf/xbar.py:28` — Declared four Queue inputs and one Queue output.
- `docs/specification/davincioo/ndf-next/interfaces/modules.md:22` — Normative ingress and completion boundaries.
- `docs/specification/davincioo/ndf-next/tile/tmu.md:69` — BGF transport must preserve identity and not report architectural completion.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
