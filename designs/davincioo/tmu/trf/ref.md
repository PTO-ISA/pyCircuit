# TMU.TRF.REF — Reference Count

- Source candidate: `DAV-TMU-TRF-REF-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trf/ref.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Sole physical-version lease ledger; logical mapping and architectural lifetime remain in RAT/FRE coordination.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| lease_acquire_req | LeaseAcquireReq | Acquire an owner/generation-qualified Tile lease | proposed |
| lease_transfer_req | LeaseTransferReq | Transfer ownership without an unowned interval | proposed |
| tile_lease_release_req | TileLeaseReleaseReq | Idempotent physical lease release | proposed |
| refcount_query_req | RefcountQueryReq | Query physical references to a TileVersion | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| lease_acquire_ack | LeaseAcquireAck | Lease acquisition result | proposed |
| lease_transfer_ack | LeaseTransferAck | Atomic transfer result | proposed |
| tile_lease_release_ack | TileLeaseReleaseAck | Idempotent release result | proposed |
| refcount_query_resp | RefcountQueryResp | Per-version physical refcount | proposed |
| reclaim_candidate | ReclaimCandidate | Zero-ref notification requiring separate logical-lifetime permission | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- LeaseTable keyed by lease slot with full TileLeaseKey
- Lease kind/count/acquired epoch/release-pending/tombstone
- Per-version refcount reduction

## Required capabilities to verify

- Associative Table with field updates
- Atomic insert/increment and transfer across rows
- Multiple outputs and no-fail commit groups
- Reduction/query over generation-qualified entries
- PYC/RTL Table lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Acquire stalls before upstream acceptance when table is full
- Transfer has no unowned interval
- Duplicate release decrements once
- Zero count emits reclaim candidate but does not free logical version
- Canceled reads retain tombstones until late responses drain

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze lease kinds, table capacity, generation wrap bound, and reduction implementation.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:250` — Detailed REF packet.
- `docs/architecture/core/l3/CONTRACT_FOUNDATION.md:104` — REF owns physical Tile lease ledger and explicit release Queue pairs.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
