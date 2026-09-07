# MEM.L2C.MROB — Memory Reorder Buffer

- Source candidate: `DAV-MEM-L2C-MROB-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `L2C`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/l2c/mrob.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Owns independently matched fragment dependencies and return maps; it is explicitly not an architectural ROB and release is not retirement.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| mrob_alloc_req | MrobAllocReq | Install complete fragment/dependency row | proposed |
| data_ready_req | DataReadyReq | Matching data-slot generation becomes ready/faulted | proposed |
| return_map_req | ReturnMapReq | Insert qualified fragment-to-beat mapping | proposed |
| mrob_to_rwdb_ack | MrobToRwdbAck | RWDB accepted complete contributing set | proposed |
| mrob_cancel_req | MrobCancelReq | Recovery/discard request | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| mrob_alloc_ack | MrobAllocAck | Allocation result | proposed |
| data_ready_ack | DataReadyAck | Matched readiness result | proposed |
| return_map_ack | ReturnMapAck | Map insertion result | proposed |
| mrob_to_rwdb_req | MrobToRwdbReq | Atomic complete beat contributors and return map | proposed |
| prior_completion | PriorCompletion | Ordered prior/cacheline completion | proposed |
| mrob_cancel_ack | MrobCancelAck | Cancel/drain result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Non-FIFO MrobTable keyed by generation-qualified MrobKey
- MemTxnKey/FragmentKey/data-slot lease
- Return-map vector and ready/fault/delivery state
- Separate per-request completion/order scoreboard

## Required capabilities to verify

- Associative Table with bounded vector fields
- Atomic many-row select/mark for one beat
- Multiple independently backpressured outputs
- Overlap/bounds verification
- PYC/RTL Table and compound commit lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Out-of-order fragments match exact entry/generation
- Return-map overlap/out-of-bounds is rejected without partial state
- RWDB capacity is reserved before contributors mark delivered
- Rows may release out of physical order after qualified delivery
- MROB release does not retire an instruction/block or publish Tile state
- Late canceled responses drain via discard acknowledgement

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze MrobKey schema/capacity, max return-map fan-in/fan-out, scoreboard policy, and DATA lease protocol.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:416` — Detailed non-FIFO MROB and explicit non-retirement semantics.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:875` — MROB release is matched delivery, separate from instruction completion.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
