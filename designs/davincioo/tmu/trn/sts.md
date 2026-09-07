# TMU.TRN.STS — Tile Status

- Source candidate: `DAV-TMU-TRN-STS-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRN`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trn/sts.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Sole descriptor, allocation, definedness, and publication-status owner for each TileVersion; raw payload stays in BANK.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| status_reserve_req | StatusReserveReq | Create an absent speculative version row | proposed |
| status_write_req | StatusWriteReq | Update descriptor/coverage metadata for same owner/generation | proposed |
| status_publish_req | StatusPublishReq | BROB-qualified publication request | proposed |
| status_rollback_req | StatusRollbackReq | Delete matching speculative row | proposed |
| status_query_req | StatusQueryReq | Read immutable status snapshot | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| status_reserve_ack | StatusReserveAck | Reservation result | proposed |
| status_write_ack | StatusWriteAck | Metadata coverage update result | proposed |
| status_publish_ack | StatusPublishAck | Architectural publication result | proposed |
| status_rollback_ack | StatusRollbackAck | Rollback result | proposed |
| status_query_resp | StatusQueryResp | StatusRow snapshot | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- StatusTable keyed by TileVersion
- Allocated/speculative/owner
- dtype/layout/shape/valid_region
- allocation_mask/initialized_mask
- element-definedness/contents_defined
- First retained pre-publication fault
- Table epoch

## Required capabilities to verify

- Associative Table with partial field/mask updates
- Atomic multi-Queue/Table events
- Bounded element-definedness representation
- Multiple independently backpressured outputs
- Hierarchical PYC/RTL Table lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Allocation does not imply payload definedness
- Blocked ack prevents request consumption and writes
- Publication validates descriptor compatibility before visible state changes
- Rollback cannot erase a newer published generation
- Shared multi-destination publication cannot expose mixed old/new records
- STS contains no payload bytes

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Select exact bounded element-definedness representation and descriptor snapshot/handle form.

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

- `docs/specification/davincioo/ndf-next/tile/tmu.md:36` — Normative descriptor/status fields and speculative ownership.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:159` — Detailed STS state and Queue packet.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:789` — Descriptor/version owner, not raw cells, retains metadata.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
