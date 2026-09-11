# TMU.TRN.LRM — Local Rename Map

- Source candidate: `DAV-TMU-TRN-LRM-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRN`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **alias** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **resolved as an alias; no module
  exists and none should**. [RAT](rat.md) is implemented and carries the Local
  projection.

Local Rename Map is only the Local projection of RAT and must not own a second mapping table.

## Where the alias lands

There is no `lrm.py`, and creating one would be the defect this disposition exists
to prevent. The alias lands in two places instead:

- **Type.** A Local request is a `MapRequest` with `scope = SCOPE_LOCAL`, declared
  in [`contracts/tmu_trn.py`](../../contracts/tmu_trn.py) alongside RAT's other
  types. There is no `LocalMapRequest`; a separate type would let the two drift
  and would make "the Local map" look like a different thing.
- **Instance.** The Local projection is [`rat.py`](rat.py) instantiated per PE.
  Per-PE isolation is structural -- the map is declared inside the system, so two
  PEs using the same logical name cannot collide -- which is exactly the property
  the LRM vocabulary names.

`scope` is carried for the parent, which uses it to select the Local or the Shared
instance. RAT never reads it, for the same reason BANK never reads `cell_key`:
whoever routes on a field must be the only one interpreting it.

`designs/davincioo/AGENTS.md` is explicit that empty Python modules must not be
created to make the inventory look implemented. An `lrm.py` that forwarded to RAT
would add a transport hop and a second name for one owner, which is what
"no added transport latency when represented as schema alias" rules out.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| local_map_request | RenameLookupReq\|MapSwapReq\|MapPublishReq\|MapRollbackReq | Local-space request forwarded to RAT | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| local_map_response | RenameLookupResp\|MapSwapAck\|MapPublishAck\|MapRollbackAck | Local-space RAT response | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- No additional item recorded; exact state/port review remains required.

## Required capabilities to verify

- Typed alias/interface projection without state duplication

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- All mutations occur in RAT — **holds**; there is no other writer, because there
  is no other module
- Local keys retain FlowKey and pe_id — **partially**; PE identity is instance
  geometry rather than a key field, so a Local request carries no `pe_id` at all.
  Whether the flow qualification RAT does carry is sufficient is part of RAT's
  open decision on transaction identity
- No added transport latency when represented as schema alias — **holds**; the
  alias is a type and an instance, not a forwarding module

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- **Whether to retain the vocabulary alias or formally retire the candidate ID.**
  The alias is currently retained because [rat.md](rat.md) and the source evidence
  both use the word, and a reader meeting "LRM" needs somewhere to land. Nothing
  in the implementation depends on the ID.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:115` — LRM is an alias/interface and never a second Local map.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
