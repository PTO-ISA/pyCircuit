# MEM.L2C.ARB — Arbiter

- Source candidate: `DAV-MEM-L2C-ARB-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `L2C`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/l2c/arb.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Owns bounded fairness and conflict resolution among L2 tag/data/return requesters, without owning their payload state.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| l2_requests[] | L2ArbReq | Qualified tag/data/return requests | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| l2_grants[] | L2ArbGrant | Accepted conflict-free grants | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Per-resource round-robin/age cursors
- Optional retained selected grant

## Required capabilities to verify

- Parameterized Queue arrays
- Explicit matching/conflict scheduling
- Accepted-transfer fairness state

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Colliding requests have one deterministic accepted winner
- Independent resources may progress concurrently
- Blocked sink retains selected ownership
- Continuously eligible clients make progress

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze resource matrix, arbitration classes, widths, and fairness bound.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:128` — L2C.ARB leaf owns bounded arbitration.
- `docs/architecture/core/l3/CONTRACT_FOUNDATION.md:205` — Fairness cursor advances on accepted ownership transfer.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
