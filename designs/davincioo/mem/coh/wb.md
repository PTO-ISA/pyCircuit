# MEM.COH.WB — Writeback

- Source candidate: `DAV-MEM-COH-WB-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `COH`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **review** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/coh/wb.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Writeback needs a dirty-data lease and terminal completion contract before it can be a safe leaf.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| writeback_req | WritebackReq | Qualified dirty data-slot/version and target | unresolved |
| writeback_terminal | TxnTerminalResp | Generation-safe downstream completion | unresolved |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| writeback_txn | TxnFragmentReq | Downstream writeback transaction | unresolved |
| writeback_ack | WritebackAck | Completion and lease-release authority | unresolved |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Dirty-data lease and retained writeback obligation are required

## Required capabilities to verify

- Cross-owner lease transfer/release
- Atomic dirty-state transition plus downstream reservation
- Retained terminal response matching

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- DATA slot remains live until writeback terminal result is accepted
- No dirty-state loss under backpressure
- Stale terminal response cannot release a reused slot

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze DATA lease protocol, writeback granularity, and ordering owner interaction.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:127` — WB deferred pending dirty-data lease and completion contract.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:542` — DATA slot cannot free while writeback holds a reference.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
