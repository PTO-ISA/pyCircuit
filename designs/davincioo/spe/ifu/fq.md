# SPE.IFU.FQ — Fill Queue

- Source candidate: `DAV-SPE-IFU-FQ-0001`
- Hardware hierarchy: **H3**, within H1 `SPE` / H2 `IFU`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/spe/ifu/fq.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Fill Queue has distinct persistent ownership, arbitration, storage, or transformation responsibility suitable for a pyCircuit design leaf.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| fill_request | IFillRequest | miss waiter request | proposed |
| memory_response | MemoryResponse | identity-qualified refill fragment | proposed |
| waiter_cancel | FetchCancel | waiter cancellation | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| memory_request | MemoryRequest | lower-memory line request | proposed |
| fill_fragment | FetchFragment | qualified waiter response | proposed |
| waiter_terminal | FillTerminal | waiter completion/fault | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- fill coalescing table, fragment mask and per-request waiters

## Required capabilities to verify

- typed compound Queue payloads
- atomic input/state/output rule semantics
- bounded Table/Array lowering with generation-qualified identity
- multi-output all-or-none publication and independent backpressure

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- All declared/proposed Queue outputs remain stable under backpressure and preserve full identity/generation.
- Stale, duplicate, wrong-flow, and post-recovery responses cause no mutation.
- gfsim and generated C++/Verilog agree at accepted Queue transfers.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- No additional item recorded; exact state/port review remains required.

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

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:1` — Catalog row: IFU.FQ, source srcs/core/spe/ifu/fq.py, status planned/deferred.
- `docs/specification/davincioo/ndf-next/scalar/ifu.md:229` — Normative NDF owner/refinement clause.
- `docs/architecture/core/l3/FRONTEND_CONTEXT_PACKETS.md:463` — Detailed proposed module card or disposition evidence.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
