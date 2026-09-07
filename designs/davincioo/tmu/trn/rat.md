# TMU.TRN.RAT — Register Alias Table

- Source candidate: `DAV-TMU-TRN-RAT-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRN`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trn/rat.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Sole logical Local/Shared name-to-version map owner; BISQ/LTR own ordering and coordination.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| rename_lookup_req | RenameLookupReq | Lookup current versions for a RenameTxnKey | proposed |
| map_swap_req | MapSwapReq | Install speculative old/new history for next ordered transaction | proposed |
| map_publish_req | MapPublishReq | Make a validated version architectural | proposed |
| map_rollback_req | MapRollbackReq | Restore exact old mapping by history generation | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| rename_lookup_resp | RenameLookupResp | Immutable current source versions | proposed |
| map_swap_ack | MapSwapAck | Idempotent speculative map-history result | proposed |
| map_publish_ack | MapPublishAck | Publication result | proposed |
| map_rollback_ack | MapRollbackAck | Generation-qualified restore result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Local maps independently projected per FlowKey/PE
- One launch/group-scoped Shared map
- MapRow valid/current/map_generation/last_published
- Bounded speculative history

## Required capabilities to verify

- Multidimensional parameterized map storage
- Associative speculative history Table
- Atomic map/history updates with multiple response paths
- Nonzero architectural reset image
- PYC/RTL state lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Four PE flows with identical logical IDs remain separate
- Shared has one map owner
- Repeated RenameTxnKey returns prior acknowledgement
- Full history stalls before FRE allocation commits
- Rollback restores only matching history generation and proceeds youngest-first

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze logical namespace sizes, reset map, speculative-history depth, and Shared namespace_scope encoding.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:195` — Detailed RAT packet.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:613` — CoreSharedArray has one RAT/FRE/STS owner.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
