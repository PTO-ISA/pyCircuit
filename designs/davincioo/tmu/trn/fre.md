# TMU.TRN.FRE — Free-list

- Source candidate: `DAV-TMU-TRN-FRE-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRN`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trn/fre.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Sole owner of free physical extents and speculative reservations for Local and Shared payload storage.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| alloc_req | AllocReq | RenameTxnKey-qualified complete extent reservation | proposed |
| commit_reservation_req | CommitReservationReq | Commit an existing reservation disposition | proposed |
| cancel_reservation_req | CancelReservationReq | Return an unpublished exact reservation | proposed |
| free_version_req | FreeVersionReq | Free after logical-lifetime permission and zero physical refs | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| alloc_ack | AllocAck | Stable TileVersion/extent or capacity result | proposed |
| commit_reservation_ack | CommitReservationAck | Disposition update result | proposed |
| cancel_reservation_ack | CancelReservationAck | Cancellation result | proposed |
| free_version_ack | FreeVersionAck | Generation-safe reclaim result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- FreeLocal extent state per PE
- One core-owned FreeShared extent state
- ReservationTable keyed by RenameTxnKey
- Reservation and allocator generations

## Required capabilities to verify

- Parameterized banked bitset/Array state
- Bounded extent selection
- Associative idempotence Table
- Atomic reserve plus response and cancel/free
- Nonzero reset initialization FSM or verified primitive

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Allocation is all-or-none
- Duplicate RenameTxnKey returns the same reservation
- PE-local exhaustion does not consume another PE's pool
- Free requires both logical permission and zero REF count
- Physical capacity exhaustion backpressures rather than inventing an architectural fault

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze extent allocation policy, physical capacities, and zero/reset image construction.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:221` — Detailed FRE packet.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:753` — Raw capacity and architectural quota are accounted separately.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
