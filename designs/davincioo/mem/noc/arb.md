# MEM.NOC.ARB — Arbiter

- Source candidate: `DAV-MEM-NOC-ARB-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `NOC`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/noc/arb.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Owns per-output/per-VC fairness for NOC transport while BUF/VC/RTR own residency and progression.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| route_requests[] | NoCGrantReq | Eligible head flits/packets targeting output/VC | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| route_grants[] | NoCGrant | Accepted per-output/VC grant | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Per-output/VC round-robin or age cursors
- Retained selected grant if required

## Required capabilities to verify

- Parameterized Queue arrays
- Explicit per-output conflict matching
- Fairness state updated on accepted transfer

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- At most one accepted owner per constrained output/VC
- Blocked destination preserves selected packet
- Continuously eligible sources make progress
- Arbitration cannot change transaction or beat identity

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze port count, VC count, packet/flit granularity, and arbitration policy.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:141` — NOC.ARB owns per-output/VC arbitration fairness.
- `docs/specification/davincioo/ndf-next/tile/memory.md:50` — NOC must retain traffic and identity under backpressure.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
