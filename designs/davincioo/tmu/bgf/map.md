# TMU.BGF.MAP — Mapping

- Source candidate: `DAV-TMU-BGF-MAP-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `BGF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/bgf/map.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Pure placement decoder with optional registered pipeline; it maps qualified physical cells without owning allocation or payload.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cell_map_req | CellMapReq | Storage scope, owner PE, physical cell index, and full request identity | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cell_map_resp | CellMapResp | Bank group/bank/row, repeated identity, mapping epoch, and bounds result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Static scope base/range tables
- Static bank_groups, banks_per_group, rows_per_bank, mapping_mode
- Optional one-row pipeline per lane

## Required capabilities to verify

- Static/JIT parameterized mapping arithmetic
- Bounded array lookup and exact-width index checks
- Optional parameterized pipeline lanes

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Consume only when response capacity is reserved
- First profile is a deterministic bijection
- Reject Local PE cross-ownership and Local/Shared aliasing
- Mapping preserves complete source identity under backpressure

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze physical cell-index width and selected Local/Shared bank geometries.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:301` — Detailed MAP packet.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:771` — Baseline bank/row mapping and DSE constraints.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
