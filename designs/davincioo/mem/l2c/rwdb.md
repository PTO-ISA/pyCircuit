# MEM.L2C.RWDB — Read Write Data Buffer

- Source candidate: `DAV-MEM-L2C-RWDB-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `L2C`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/l2c/rwdb.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Owns Tile beat reassembly and TMU request residency, but never destination definedness or architectural publication.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| mrob_to_rwdb_req | MrobToRwdbReq | Qualified miss fragments and return map | proposed |
| hit_to_rwdb_req | HitToRwdbReq | Qualified hit snapshot and return map | proposed |
| tile_read_resp | TileReadResp | Source Tile bytes for TSTORE/writeback | proposed |
| tile_write_ack | TileWriteAck | Physical TMU write completion | proposed |
| rwdb_cancel_req | RwdbCancelReq | Generation-qualified recovery | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| mrob_to_rwdb_ack | MrobToRwdbAck | Accepted contributing miss fragments | proposed |
| hit_to_rwdb_ack | HitToRwdbAck | Accepted hit fragment | proposed |
| tile_read_req | TileReadReq | Generation-qualified source Tile beat request | proposed |
| tile_write_req | TileWriteReq | Complete mapped destination Tile beat | proposed |
| beat_complete | BeatComplete | Physical beat result/fault, not EngineDone | proposed |
| rwdb_cancel_ack | RwdbCancelAck | Cancel/drain result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- RwdbTable keyed by flow-qualified RwdbKey
- CommandKey/TileVersion/beat ID/source kind
- Expected and received byte masks
- 128-byte data and first fault
- TMU request ID and write-ack state

## Required capabilities to verify

- Masked aggregate merge into Table/Array
- Atomic fragment acceptance and coverage update
- Multiple independent Queue ports
- Retained outstanding TMU request FSM
- PYC/RTL state lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Unequal cacheline/cell/return-beat widths reassemble each byte exactly once
- Duplicate fragments are idempotent exact matches or typed faults
- Complete entry remains resident while TileWriteReq is blocked
- BeatComplete follows physical TMU ack but does not publish STS or complete block
- Cancellation retains destination version until outstanding TMU response drains

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze beat width, RwdbKey/capacity, return-map schema, and direct TMU interface types.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:441` — Detailed RWDB packet.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:883` — RWDB release requires TMU complete-beat acceptance and acknowledgement.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
