# TMU.TRF.BANK — Register Bank

- Source candidate: `DAV-TMU-TRF-BANK-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/trf/bank.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Sole owner of raw payload bytes for one physical bank; descriptor and publication metadata remain in STS/RAT.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cell_read_req | CellReadReq | CellKey/TileVersion-qualified masked read with request ID and response route | proposed |
| cell_write_req | CellWriteReq | CellKey/TileVersion-qualified masked shadow write | proposed |
| bank_invalidate_req | BankInvalidateReq | Generation-qualified cell invalidation/reset work | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cell_read_resp | CellReadResp | Snapshotted bytes or typed stale/fault result | proposed |
| cell_write_ack | CellWriteAck | Physical write acknowledgement, not publication | proposed |
| bank_invalidate_ack | BankInvalidateAck | Invalidation completion | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Payload Array[rows_per_bank, UInt[1024]] for 128-byte cells
- CellGeneration per row
- WriteBusy
- PendingRead pipeline sized by read_latency

## Required capabilities to verify

- Banked Array with masked writes
- Atomic Queue consume plus Array update plus ack
- Multiple independent input/output Queues
- Parameterized read-latency pipeline
- PYC/RTL lowering for stateful Array hierarchy

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Baseline services at most one access per cycle because selected banks are single-port
- A blocked response/pipeline slot prevents read grant
- Masked write preserves untouched bytes
- Old CellKey generation cannot alter a reused row
- BANK never changes descriptor/definedness or publishes Tile state

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze bank/row counts, read latency, Queue depths, and whether any profile supports more than one configured access per bank.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:277` — Detailed BANK packet and raw payload ownership.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:738` — Selected proposal derives from 32 single-port 128-byte banks; first profile uses private groups.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:789` — Descriptor fields explicitly excluded from raw SRAM cell ownership.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
