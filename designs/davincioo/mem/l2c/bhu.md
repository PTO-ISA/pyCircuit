# MEM.L2C.BHU — Bank Hazard Unit

- Source candidate: `DAV-MEM-L2C-BHU-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `L2C`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/l2c/bhu.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Owns lower-memory bank hazards, downstream IDs, and terminal response lifecycle.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| bhu_req | BhuReq | Generation-qualified downstream memory request | proposed |
| downstream_resp | DownstreamMemResp | Terminal/retry/fault response | proposed |
| bhu_cancel_req | BhuCancelReq | Cancellation or discard-on-return request | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| downstream_req | DownstreamMemReq | Hazard-admitted request to NOC/external memory | proposed |
| bhu_resp | BhuResp | Generation-safe terminal response to TXN | proposed |
| bhu_cancel_ack | BhuCancelAck | Cancellation/drain result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Downstream transaction IDs and generations
- Per-bank/address hazard records
- Outstanding request/terminal response state

## Required capabilities to verify

- Associative hazard Table
- Free-ID allocator with generations
- Multiple Queue atomic events
- Retained external-response FSM

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Hazards serialize only required requests
- Terminal response is delivered before ID reuse
- Canceled work drains safely
- Stale generation never completes a new request

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze downstream bus-independent request schema, hazard granularity, and capacity.

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

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:129` — BHU owner recommendation.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:877` — BHU owns downstream IDs/hazards until terminal generation-safe release.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
