# TMU.TRF.CELL — Tile Cell

- Source candidate: `DAV-TMU-TRF-CELL-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **state_schema** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **implemented as contained state**, in
  the two arrays [bank.py](bank.py) elaborates plus the geometry constants in
  [contracts/tmu_trf.py](../../contracts/tmu_trf.py). There is deliberately no
  `cell.py`: this disposition authorises no independent leaf, and a module here
  would create a second owner of the same bytes.

Defines one raw 128-byte payload cell and generation; it is not a separately scheduled module or descriptor owner.

## Containing owner

| Element | Where it lives | Note |
| --- | --- | --- |
| 128-byte payload | `cells` array in [bank.py](bank.py) | **One entry holds one whole cell.** The element is the `CellData` struct, whose lowered size is exactly 128 bytes |
| Cell generation | `generations` array in [bank.py](bank.py) | One entry per row, since a generation qualifies a whole cell |
| Layout geometry | [contracts/tmu_trf.py](../../contracts/tmu_trf.py) | `ROWS_PER_BANK`, `CELL_BYTES`, `WORDS_PER_CELL`, `CELL_READ_LATENCY` |

## Inputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

## Outputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- 128-byte payload
- Cell generation
- No allocation, shape, dtype, layout, valid-region, or definedness fields

## Required capabilities to verify

- Exact aggregate/array schema for 1024 payload bits — **available**; the cell
  is one flat struct of sixteen `u64` fields stored in one entry of a persistent
  indexed variable (Decision 0151). Those fields are a representation of one
  indivisible payload, not an addressable coordinate
- Masked subword access representation — **not required**; access granularity is
  one whole cell, so no subword access exists

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Schema is embedded in BANK-owned Array — **holds**; the two arrays are
  elaborated by BANK and no other owner exists
- CellKey never substitutes for TileVersion/TileLease — **holds structurally**;
  BANK never reads `cell_key` at all. Addressing uses `row` and the generation
  decision uses `tile_version`, so the two cannot be confused even by mistake
- Allocation does not imply payload definedness — **not yet provable here**; the
  cell array initialises to zero and BANK carries no definedness state, so nothing
  distinguishes "allocated" from "genuinely written to zero". Proving it needs the
  allocator, [fre.md](../trn/fre.md), and an owner for definedness. Neither is
  implemented. Note the allocator is FRE, not ALC: ALC does not allocate

Design-local evidence: `designs/davincioo/tests/fabric/test_bank_physical_access.py`
(`test_cell_key_never_substitutes_for_tile_version`,
`test_cell_schema_lives_in_bank_owned_arrays_only`).

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- **PYC/RTL representation of the 1024-bit payload is still open.** The pyCircuit
  representation is settled: one entry, one flat struct, no byte mask, since
  access granularity is one whole cell and no sub-cell extent is requestable.
  But that storage is provisional state under Decision 0151, which PYC/RTL must
  reject, so a lowerable representation still depends on the aggregate-memory
  framework fix; see [bank.md](bank.md) for the boundary and its reproducer.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:110` — CELL is a state schema, not a second owner.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:729` — Raw cells and Tile metadata have different owners.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
