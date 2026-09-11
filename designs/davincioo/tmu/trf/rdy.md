# TMU.TRF.RDY — Readiness

- Source candidate: `DAV-TMU-TRF-RDY-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TRF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **state_schema** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **not implemented, and no `rdy.py`
  should exist**. This disposition authorises no independent leaf, and the first
  behavioral acceptance below forbids the state such a leaf would hold; see
  "Open decisions".

Readiness is a projection derived from STS coverage and BANK acknowledgements, not an independent mutable truth.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| status_snapshot | StatusQueryResp | Descriptor and definedness status | proposed |
| bank_completion | CellWriteAck\|CellReadResp | Physical completion used to derive readiness | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| readiness | TileReadiness | Qualified readiness projection for a TileVersion and requested region | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- No additional item recorded; exact state/port review remains required.

## Required capabilities to verify

- Derived schema/view support or stateless combinational projection

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Never owns a second ready bit that can diverge from STS/BANK
- Readiness remains TileVersion and region qualified
- Physical completion alone is not architectural publication

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Choose whether readiness is computed by STS query response or represented as a
  reusable interface schema. **Neither option is an independent leaf**, so this
  decision selects a containing owner, not an implementation path. A separate
  `rdy.py` would own a second ready bit that can diverge from STS and BANK, which
  the first behavioral acceptance forbids; readiness is a projection, so its only
  correct forms are a computation inside its containing owner or a shared type.
- Resolving it depends on [sts.md](../trn/sts.md), which is not implemented. Until
  STS exists there is no query response to project from and no owner to host the
  computation, so this candidate is blocked rather than merely unclaimed.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:111` — RDY is explicitly a STS/BANK acknowledgement-derived projection.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
