# MEM.NOC.RTR — Router

- Source candidate: `DAV-MEM-NOC-RTR-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `NOC`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **review** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/noc/rtr.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

NOC routing owner is required, but candidate remains assembly/leaf until topology, BUF/VC ownership, and route progression boundary are frozen.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| router_ingress[] | NoCFlit\|NoCPacket | Per-port transport ingress | unresolved |
| credit_or_accept[] | NoCFlowControl | Destination/VC capacity feedback if selected | unresolved |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| router_egress[] | NoCFlit\|NoCPacket | Per-port routed transport output | unresolved |
| transport_status | NoCTransportStatus | Local delivery/cancel status only | unresolved |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Route progression
- Router-owned BUF entries
- Potential VC allocation/credit references
- Per-packet/flit retained identity

## Required capabilities to verify

- Parameterized cyclic module graph
- Multiple Queue arrays
- Static route function
- Atomic VC/ARB/XBAR coordination
- Potential credit-flow primitive after protocol selection

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Preserve transaction/beat/agent/flow/order identity
- Stale response cannot complete reused transaction
- Transport delivery cannot satisfy instruction or block resolve
- All blocked traffic remains resident

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Owner must choose assembly versus leaf, topology/routing, flit format, VC count, and flow-control scheme.

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

- `docs/specification/davincioo/ndf-next/tile/memory.md:50` — Normative NOC identity/backpressure boundary.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:143` — RTR remains assembly/leaf recommendation.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
