# TMU.TRF.ALC — Allocation

- Source candidate: `DAV-TMU-TRF-ALC-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **interface** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **not implemented, and deliberately
  so**. No capacity-query consumer exists, so the recorded fallback applies and
  this candidate should collapse into FRE's interface; see "Open decisions".

Allocation ownership belongs to FRE; ALC may only adapt capacity queries at the TRF boundary.

**ALC does not allocate.** The name is the work item's, not a description of its
behaviour: allocation state, reservations and allocator generations all belong to
[fre.md](../trn/fre.md). ALC's entire proposed scope is one stateless capacity
projection that reserves nothing.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| capacity_query | TrfCapacityQuery | Scope/PE-qualified physical capacity query | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| capacity_response | TrfCapacityResponse | Available geometry/capacity projection without reservation | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- No additional item recorded; exact state/port review remains required.

## Required capabilities to verify

- Typed interface aliases or stateless adapter modules

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Never allocates or mutates FRE state
- Does not translate physical exhaustion into a guest allocation fault
- Returns scope-qualified capacity

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- **No runtime capacity-query consumer exists in this design program, so the
  recorded fallback applies: collapse this candidate into FRE's interface.**
  Searching `designs/davincioo/` for `capacity_query`, `TrfCapacityQuery` and
  `capacity_response` returns only this card and its Chinese counterpart. No
  module declares either port as a producer or a consumer. Meanwhile
  [fre.md](../trn/fre.md) already returns a capacity result on `alloc_ack`
  ("Stable TileVersion/extent or capacity result"), which is the path a caller
  actually has. Deleting the candidate needs registry approval, so it stays
  recorded here rather than removed.
- Consequently **no `alc.py` is authorised or useful**. This disposition
  authorises no independent leaf, ALC owns no state, and a stateless adapter with
  no consumer on either side would be an empty module. Work that looks like
  allocation belongs in [fre.md](../trn/fre.md).

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:108` — ALC is an interface/helper and FRE owns allocation.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
