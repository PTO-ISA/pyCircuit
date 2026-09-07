# MEM.L2C.DATA — Data Array

- Source candidate: `DAV-MEM-L2C-DATA-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `L2C`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/l2c/data.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Single owner of L2 payload slots, readiness/dirty state, generations, and slot references.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| data_read_req | DataReadReq | Qualified data-slot/generation read | proposed |
| data_write_req | DataWriteReq | Masked refill/store/writeback update | proposed |
| data_lease_req | DataLeaseReq | Acquire/release TAG/MROB/RET/writeback/BHU reference | proposed |
| data_invalidate_req | DataInvalidateReq | Detach or invalidate exact slot generation | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| data_read_resp | DataReadResp | Qualified payload snapshot/status | proposed |
| data_write_ack | DataWriteAck | Write completion | proposed |
| data_lease_ack | DataLeaseAck | Reference transition result | proposed |
| data_invalidate_ack | DataInvalidateAck | Generation-safe invalidation result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Payload Array by data slot
- Slot generation
- Ready and dirty masks
- Reference counts/leases
- Replacement/writeback status

## Required capabilities to verify

- Banked/masked Array
- Atomic Array plus metadata Table updates
- Multi-owner lease accounting
- PYC/RTL state hierarchy

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- TAG replacement only detaches mapping
- Slot frees only after all tag/MROB/RET/writeback/BHU obligations release
- Old generation cannot read/write/free reused payload
- Backpressure cannot drop dirty data

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze line width, slot count, bank/port geometry, mask representation, and lease schema.

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

- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:877` — DATA payload-slot ownership and release condition.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:542` — DATA is the single L2 payload-slot owner; other modules hold leases.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
