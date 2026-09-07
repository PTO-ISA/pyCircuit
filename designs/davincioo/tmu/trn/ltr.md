# TMU.TRN.LTR — Logical Tile Register

- Source candidate: `DAV-TMU-TRN-LTR-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRN`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trn/ltr.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Retained lifetime coordinator that exposes one ordered TileRename transaction while STS/RAT/FRE/REF remain sibling state owners.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tile_rename_req | TileRenameReq | BISQ-ordered rename with stable RenameTxnKey and exact scope | proposed |
| tile_publish_req | TilePublishReq | BROB-qualified compatible destination publication | proposed |
| recovery_req | TileRecoveryReq | Flow/epoch-qualified cleanup | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tile_rename_ack | TileRenameAck | Retained source versions, shadow destinations, prior-map handles, lease obligations | proposed |
| tile_publish_ack | TilePublishAck | Atomic descriptor/map publication acknowledgement | proposed |
| recovery_ack | TileRecoveryAck | Cleanup completion after sibling acknowledgements | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Lifetime transaction FSM keyed by RenameTxnKey
- Phase and sibling acknowledgement mask
- Retained reservation and cleanup handles

## Required capabilities to verify

- Multi-input/multi-output rules
- Compound Queue/Table/Reg no-fail commit groups
- Retained multi-cycle transaction FSM
- Explicit sibling conflict scheduling
- Hierarchical stateful gfsim and PYC/RTL lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- BISQ rename order is preserved
- Retry never allocates a second destination
- Zero-mask/no-effect bypasses allocation and readiness waits
- No map/status publication before all resources and sink capacity are secured
- Publication is separate from rename and atomically validates compatible destinations
- Physical read release remains distinct from architectural lifetime finalization

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze complete TileRenameReq/Ack and multi-destination publication schemas.

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

- `docs/specification/davincioo/ndf-next/tile/tmu.md:18` — Normative identity-qualified lifetime and ordered TileRenameReq/Ack boundary.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:500` — LTR coordinator and parent-owned sibling Queues.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
