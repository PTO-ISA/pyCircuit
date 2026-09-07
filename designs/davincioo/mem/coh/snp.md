# MEM.COH.SNP — Snoop

- Source candidate: `DAV-MEM-COH-SNP-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `COH`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **review** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/coh/snp.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

A snoop leaf requires a selected coherence agent and response contract, which current architecture deliberately leaves open.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| snoop_req | SnoopReq | Coherence-agent-qualified probe | unresolved |
| snoop_data_resp | SnoopDataResp | Data/status from local cache owner | unresolved |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cache_probe | CacheProbeReq | Local qualified probe | unresolved |
| snoop_resp | SnoopResp | Protocol response with matching maintenance/transaction identity | unresolved |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Coherence transient and response obligation are protocol-dependent and unresolved

## Required capabilities to verify

- Protocol enums and compound response schemas
- Retained request/response FSM
- External integration adapter

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Snoop completion preserves agent and operation identity
- Internal transient never masquerades as PTO maintenance completion

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Select coherence protocol/agent boundary and snoop response states.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:126` — SNP deferred until coherence-agent and response contract exist.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:933` — AXI versus CHI and external coherence remain integration choices.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
