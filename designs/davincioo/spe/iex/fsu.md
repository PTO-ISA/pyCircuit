# SPE.IEX.FSU — Floating-point and SIMD Unit

- Source candidate: `DAV-SPE-IEX-FSU-0001`
- Hardware hierarchy: **H3**, within H1 `SPE` / H2 `IEX`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **review** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/spe/iex/fsu.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Floating-point and SIMD Unit lacks a closed semantic or physical profile and must remain non-executable until reviewed.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| execute_request | ExecutePacket | FP/SIMD request; semantics unresolved | unresolved |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| terminal_result | TerminalResult | result/fault; format unresolved | unresolved |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- State shape is unresolved pending the named architecture/profile decision.

## Required capabilities to verify

- typed compound Queue payloads

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Close the listed open architecture/profile question before registering executable behavior.
- All declared/proposed Queue outputs remain stable under backpressure and preserve full identity/generation.
- Stale, duplicate, wrong-flow, and post-recovery responses cause no mutation.
- gfsim and generated C++/Verilog agree at accepted Queue transfers.

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Architecture/profile semantics for IEX.FSU are not closed; ports and state remain unresolved.
- Confirm PTO scalar FP/SIMD operations, lane formats and exception semantics.

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

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:1` — Catalog row: IEX.FSU, source srcs/core/spe/iex/fsu.py, status planned/deferred.
- `docs/specification/davincioo/ndf-next/scalar/iex.md:134` — Normative NDF owner/refinement clause.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
