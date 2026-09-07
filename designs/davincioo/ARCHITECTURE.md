# DavinciOO architecture and hardware hierarchy

This design program uses two independent classifications. The maintainer's
2026-09-07 clarification replaces the old hardware L1/L2/L3 terminology with
H1/H2/H3. It does not change PTO architectural behavior or assign new NDF IDs.

## NDF refinement and hardware composition

| Axis | Level | Meaning | Example |
| --- | --- | --- | --- |
| NDF refinement | L0 | Architectural intent: why the machine exists and what it must achieve | Execute the selected PTO program coherently across scalar, Tile, memory and group work |
| NDF refinement | L1 | Observable behavior: state, event order, results, faults and recovery | Preserve the architectural-state projection and ordered PTO commit-event stream |
| NDF refinement | L2 | Microarchitecture details implementing L1 | Queues, owners, buffers, pipelines, arbitration, capacities and timing |
| Hardware hierarchy, within NDF L2 | H1 | Functional domain | SPE, SMT, TMU, VEC, CUBE, MEM, GPE |
| Hardware hierarchy, within NDF L2 | H2 | Subsystem composed within an H1 domain | SPE.OOO, TMU.TRF, MEM.L2C |
| Hardware hierarchy, within NDF L2 | H3 | Leaf candidate or contained mechanism within an H2 subsystem | SPE.OOO.ROB, TMU.TRF.BANK, MEM.L2C.MROB |

```text
NDF L0 architectural intent
  -> NDF L1 observable behavior
     -> NDF L2 microarchitecture
        H1 functional domains
          H2 subsystems
            H3 leaves / contained state / interfaces / aliases under review
```

An H1 domain is not an NDF L1 behavior clause. An H3 leaf is not an NDF L3
refinement. A whole-core or H1 composition is still microarchitectural design
under NDF L2. Cache names such as L1D and L2C retain their cache-level meaning.

Concrete identities remain unchanged: `DAV-SPE-OOO-ROB-0001` is still the same
source candidate. Its namespace template is now described as
`DAV-{H1}-{H2}-{H3}-{NNNN}`. NDF refinement is a separate property and must
never be inferred from the number of namespace components.

## Architectural intent and behavior anchors

The imported source corpus already separates functional identity from NDF
refinement in `ndf-next/README.md`. It provides these common anchors:

- L0: `ndf://davincioo/DAV-SYS-TOP-CORE-0001`, one coherent fetch-to-retirement
  implementation path.
- L1: `ndf://davincioo/DAV-SYS-PTO-STATE-0001`, the PTO architectural-state
  projection; private queues/maps/replay state do not create architectural state.
- L1: `ndf://davincioo/DAV-SYS-PTO-CEV-0001`, ordered PTO commit events;
  pipeline stalls and internal replay are not architectural events.

These are common constraints, not a complete per-module traceability proof.
Each H3 implementation must identify its specific L1 behaviors and show how
its L2 states, rules and tests implement them. Existing source clauses tagged
with legacy refinement levels require semantic review; do not mechanically
relabel every source `L3` clause as L2 or invent `refines` edges.

## Microarchitecture selected for contributor work

The frozen reference snapshot proposes four independent PE control flows
executing the same PTO ELF entry, qualified by
`FlowKey(core_id, pe_id, stid, launch_generation)`. Each flow retains its own
PC, scalar state, ordering and recovery. Mapping that profile to the pinned
PTO state remains a recorded conformance question; changing hierarchy labels
does not resolve it.

| H1 domain | Responsibility and external transaction families |
| --- | --- |
| SPE | Fetch/decode/rename, scalar execution and memory issue, micro-commit, block admission and BROB-ordered publication |
| SMT | Context lifecycle, per-flow selection, resource admission and predicate context; independent flow identity survives sharing |
| TMU | Tile descriptor/version ownership, raw bank storage, bank access and publication/reclaim protocols |
| VEC | Retained elementwise/reduction/shuffle/SFU operations, operand access, result staging and terminal resolution |
| CUBE | Operand staging/layout, matrix accumulation, conversion and result writeback |
| MEM | Tagged scalar/Tile transaction service, streaming cache, external traffic and maintenance |
| GPE | Participant-qualified group issue, movement, rendezvous and all-required-participant completion |

SPE's main path is fetch/IB -> D1 -> scalar rename -> issue/execute ->
ROB/CMT micro-commit handoff -> BROB-ordered architectural publication.
Tile commands pass through CMD IQ/CMDP and BISQ-ordered Tile rename before
engine issue. VEC/CUBE/MEM/GPE return tagged resolve events; TMU controls
versioned Tile storage. Transport completion, micro-commit and architectural
publication remain distinct events.

Parent assemblies own the connections between children. A child never reads
or mutates sibling state directly. One compiler-inferred rule defines one
local atomic transaction; multi-cycle protocols retain phase and acknowledgement
state rather than pretending that the whole core commits as one rule.

## Ownership conflicts that must be resolved before implementation

- Resident IQ state must not be duplicated by the IQ/ISQ/S2/S3 names.
- Scalar T/U ownership belongs to SPE.OOO; TMU.TUL names are migration work
  items, not permission to create a second rename/retirement owner.
- ROB and BROB have different micro-commit/block-publication duties. MROB is
  memory dependency state and has no architectural retirement authority.
- Integrated architecture retains BIFU/BFU/F5 as legacy comparison names,
  while a scalar packet still proposes a BFU owner. Those cards retain the
  disagreement and do not authorize a second fetch/decode/BROB allocator.
- TMU raw CELL/BANK payload ownership is separate from descriptor-definedness
  and publication state. Cache/predictor geometry and group rendezvous keys
  require explicit profiles.

The candidate catalog covers 240 names, not 240 accepted independent owners.
Every name has a checklist card. A state-schema, interface or alias task may
close by implementing its accepted containing owner and documenting the
mapping; it must not manufacture another mutable state instance.

## Migration from the external terminology

| Legacy source representation | New design-program representation |
| --- | --- |
| Hardware L1/L2/L3 prose | Hardware H1/H2/H3 prose |
| Hardware catalog `l1`, `l2`, `l3` fields | `h1`, `h2`, `h3` fields |
| Module path `srcs/core/<l1>/<l2>/<l3>.py` | `designs/davincioo/<h1>/<h2>/<h3>.py` for an accepted leaf |
| Legacy `docs/architecture/core/l3/` sources | Frozen provenance paths; new task cards live in the design hierarchy |
| NDF `refinement` | Independent L0/L1/L2 semantics; no depth-derived conversion |

The source snapshot's original filenames and IDs are preserved in provenance.
The external dirty checkout is not bulk-renamed by this catalog change.
Contributors migrating its authored NDF/vocabulary tooling must update readers,
generators and validation together and retain stable IDs. The new design
catalog requires no external checkout to validate.
