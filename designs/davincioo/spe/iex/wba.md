# SPE.IEX.WBA — Writeback Arbiter

- Source candidate: `DAV-SPE-IEX-WBA-0001`
- Hardware hierarchy: **H3**, within H1 `SPE` / H2 `IEX`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/spe/iex/wba.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

A typed NDF Queue contract exists and may have a Python design draft, but the catalog still says planned/deferred; promote only after behavioral and backend gates.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| alu_result | TerminalResult | retained ALU producer lane | declared |
| bru_result | TerminalResult | retained BRU producer lane | declared |
| lsu_result | TerminalResult | retained LSU producer lane | declared |
| other_result | TerminalResult | retained DIV/FSU/SYS/CMD lane | declared |
| apply_ack | WritebackApplyAck | exact apply success or retry | declared |
| cancel | IssueCancel | local unpublished cancellation | declared |
| drain_request | WbaDrainRequest | producer quiescence proof | declared |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| commit | WritebackCommit | frozen result and effect mask | declared |
| ack_result | WritebackAckResult | apply response classification | declared |
| cancel_ack | WritebackCancelAck | cancel ownership classification | declared |
| drain_ack | WbaDrainAck | WBA release proof | declared |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- retained producer/result slots and apply/cancel/drain ownership

## Required capabilities to verify

- typed compound Queue payloads
- atomic input/state/output rule semantics
- bounded Table/Array lowering with generation-qualified identity

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

- `docs/specification/davincioo/ndf-next/interfaces/pycircuit-module-catalog.json:1` — Catalog row: IEX.WBA, source srcs/core/spe/iex/wba.py, status planned/deferred.
- `docs/specification/davincioo/ndf-next/scalar/iex.md:154` — Normative NDF owner/refinement clause.
- `docs/specification/davincioo/ndf-next/modules/spe/iex/wba.md:52` — Typed Queue/state contract for existing source draft.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
