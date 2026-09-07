# MEM.COH.INV — Invalidate

- Source candidate: `DAV-MEM-COH-INV-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `COH`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **review** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/coh/inv.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Potential invalidation leaf, but qualified fanout and acknowledgement protocol are not frozen.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| invalidate_req | MaintenanceInvalidateReq | Operation/epoch/address-scope invalidation | unresolved |
| owner_ack[] | MaintenanceOwnerAck | Affected-owner acknowledgements | unresolved |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| invalidate_work[] | MaintenanceWork | Qualified invalidation fanout | unresolved |
| invalidate_ack | MaintenanceAck | Joined architectural maintenance completion | unresolved |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Maintenance transaction identity, fanout bitmap, and acknowledgement bitmap are required

## Required capabilities to verify

- Atomic fanout reservation
- Retained acknowledgement join
- Generation-qualified cancellation/reset

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- No maintenance completion before every affected owner acknowledges
- Translation/cache requests cannot cross epochs incorrectly
- No unqualified clearing of unrelated state

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze invalidation targets, address/scope schema, and relation to reset versus recovery.

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

- `docs/specification/davincioo/ndf-next/tile/memory.md:76` — Normative coherence/maintenance boundary.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:125` — INV is deferred pending qualified fanout and acknowledgement.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
