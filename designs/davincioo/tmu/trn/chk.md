# TMU.TRN.CHK — Checkpoint

- Source candidate: `DAV-TMU-TRN-CHK-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRN`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **review** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trn/chk.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Checkpoint mechanism is required, but ownership is unresolved between a standalone leaf and RAT-private speculative-history state.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| checkpoint_req | TileCheckpointReq | Capture flow-qualified RAT history frontier | proposed |
| unwind_req | TileUnwindReq | Recover a flow to an exact checkpoint/generation | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| checkpoint_ack | TileCheckpointAck | Stable checkpoint handle | proposed |
| unwind_ack | TileUnwindAck | Youngest-first unwind completion | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Bounded flow-qualified checkpoint records
- History frontier and unwind phase if separate leaf

## Required capabilities to verify

- Bounded Table/stack semantics
- Multi-cycle acknowledged recovery transaction
- Atomic coordination with RAT/FRE/STS through parent Queues

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Recovery unwinds youngest-first
- A stale checkpoint cannot restore a newer mapping
- No direct mutation of sibling RAT/FRE/STS state

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Owner must choose standalone CHK leaf versus RAT-private state before registration.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:113` — Disposition explicitly remains leaf or RAT-private state.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:216` — RAT speculative history stalls and unwinds youngest-first.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
