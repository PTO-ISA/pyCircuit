# MEM.MIF.RSP — Response

- Source candidate: `DAV-MEM-MIF-RSP-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `MIF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/mif/rsp.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Routes terminal results by explicit typed return target; IDs such as data_slot or mrob_ref alone are insufficient.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| terminal_resp | TxnTerminalResp | Matched TXN terminal data/ack/fault with complete identity | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| scalar_result | ScalarMemResult | Scalar LSU result/fault | proposed |
| cache_fill | CachelineFill | L1I/L1D refill response | proposed |
| tile_fragment | TileMemoryFragment | MROB/RWDB Tile return | proposed |
| store_or_writeback_ack | MemoryAck | Qualified write terminal acknowledgement | proposed |
| maintenance_resp | MaintenanceResp | Qualified maintenance result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Retained selected output and terminal response while target is backpressured

## Required capabilities to verify

- Typed route over closed return-target enum
- Multiple independently backpressured outputs
- Atomic input consume with exactly one output

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Return target is explicit and typed
- Preserve InstKey/CommandKey, agent, transaction generation, fragment/beat, fault, and payload
- Blocked target retains complete response
- Terminal routing does not publish instruction/block completion

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze exact output Queue partition and return-target enum.

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

- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:898` — Return classes and typed return_target requirements.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:139` — RSP routes terminal results by qualified return target.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
