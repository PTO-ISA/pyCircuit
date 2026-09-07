# MEM.L2C.RET — Return Path

- Source candidate: `DAV-MEM-L2C-RET-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `L2C`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/l2c/ret.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Owns hit-return, bypass obligations, complete cacheline snapshots, and terminal routing; BYPQ remains private RET state.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tag_hit_req | TagHitReturnReq | TAG hit/write obligation with data-slot generation and return map | proposed |
| data_read_resp | DataReadResp | Qualified cacheline snapshot | proposed |
| bypass_ack | BypassAck | Consumer accepted a qualified return | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| data_read_req | DataReadReq | Acquire/read qualified DATA snapshot | proposed |
| hit_to_rwdb_req | HitToRwdbReq | Tile return-map fragment to RWDB | proposed |
| terminal_return | TerminalReturn | Typed scalar/cache/Tile/ack/fault return target | proposed |
| hit_beat_complete | BeatComplete | Hit-side beat completion to instruction join | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Private BypqTable of hit-return obligations
- DATA lease/qualified return target
- Retained return payload under backpressure

## Required capabilities to verify

- Private associative Table inside module
- Atomic DATA lease and output reservation
- Multiple typed return outputs
- PYC/RTL Table lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- BYPQ is private RET state, not an unregistered leaf
- Qualified return ack releases obligation
- Return target and all original identities are retained
- Hit return does not imply architectural completion

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze BypqTable entry schema/capacity and exact return-class Queue split.

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

- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:882` — RET.BypqTable ownership and release condition.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:893` — Selected profile folds BYPQ into RET.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
