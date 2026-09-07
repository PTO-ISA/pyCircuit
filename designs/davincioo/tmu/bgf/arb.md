# TMU.BGF.ARB — Arbiter

- Source candidate: `DAV-TMU-BGF-ARB-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `BGF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/bgf/arb.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Own fairness, aging, and conflict-free grants; Queue residency remains in parent-owned RQ/WQ state.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| bank_grant_requests[] | BankGrantReq | Mapped CUBE/VEC/TLSU/maintenance requests grouped by bank | proposed |
| grant_cancel | GrantCancelReq | Cancel an unaccepted generation-qualified grant | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| client_grants[] | BankGrant | Accepted per-client grant | proposed |
| grant_to_xbar | BankGrant | Same accepted grant transferred to XBAR | proposed |
| grant_cancel_ack | GrantCancelAck | Cancellation result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Per-bank requester ages
- Requester and bank round-robin cursors
- Optional class deficit counters
- One retained grant register per bank group

## Required capabilities to verify

- Atomic consume plus two-output publish
- Explicit conflict scheduling across requesters and single-port banks
- Parameterized Queue arrays
- Reg/Array state with accepted-transfer cursor updates

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- At most one accepted access per single-port bank per cycle
- Blocked grant outputs retain Queue head, payload, and fairness ownership
- Continuously eligible requesters eventually receive accepted grants
- Recovery cancels only work not accepted by XBAR

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze requester count, urgent-age threshold, class policy, and per-bank issue width.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:322` — Detailed ARB purpose, state, ports, atomic grant rule, and backpressure.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:744` — Selected Local topology uses single-port banks and aging arbitration.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
