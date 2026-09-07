# SPE.BCTRL.BFU — Block Fetch Unit

- Source candidate: `DAV-SPE-BCTRL-BFU-0001`
- Hardware hierarchy: **H3**, within H1 `SPE` / H2 `BCTRL`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **review** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/spe/bctrl/bfu.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

The execution packet proposes BFU as a module, but the integrated architecture retains BFU only for legacy comparison and forbids a second fetch/decode or BROB allocator; this checklist does not authorize a new BFU state owner.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| prediction | PredictionSidecar | selected/corrected frontend prediction | unresolved |
| redirect | RecoveryRedirect | backend-authoritative target | unresolved |
| cancel | FetchCancel | stale block work | unresolved |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| block_fetch | BlockFetchRequest | identity-qualified block launch | unresolved |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- No new BFU state is authorized; any retained legacy-comparison state and its sole owner require architecture review.

## Required capabilities to verify

- typed compound Queue payloads
- atomic input/state/output rule semantics
- multi-output all-or-none publication and independent backpressure

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- No BFU implementation or second fetch/decode/BROB-allocation state owner is created until the integrated-architecture conflict is resolved.
- All declared/proposed Queue outputs remain stable under backpressure and preserve full identity/generation.
- Stale, duplicate, wrong-flow, and post-recovery responses cause no mutation.
- gfsim and generated C++/Verilog agree at accepted Queue transfers.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Resolve whether BFU is removed/aliased for legacy comparison or retained as a module without owning any second fetch, decode, or BROB-allocation state.

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

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:1` — Catalog row: BCTRL.BFU, source srcs/core/spe/bctrl/bfu.py, status planned/deferred.
- `docs/specification/davincioo/ndf-next/scalar/bctrl.md:66` — Normative NDF owner/refinement clause.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:152` — Integrated hierarchy retains BFU/BIFU/F5 only as legacy comparison candidates and forbids a second fetch/decode or BROB allocator.
- `docs/architecture/core/l3/SPE_EXECUTION_PACKETS.md:123` — Earlier execution-packet inventory proposes BFU as a module, creating the disposition conflict.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
