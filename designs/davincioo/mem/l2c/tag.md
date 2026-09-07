# MEM.L2C.TAG — Tag Array

- Source candidate: `DAV-MEM-L2C-TAG-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `L2C`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/l2c/tag.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Sole owner of lookup tags, qualified mappings to DATA slots, and replacement metadata; never owns payload bytes.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tag_lookup_req | TagLookupReq | Address/agent/generation-qualified lookup | proposed |
| tag_install_req | TagInstallReq | Attach tag to qualified DATA slot generation | proposed |
| tag_invalidate_req | TagInvalidateReq | Detach exact tag mapping | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tag_lookup_resp | TagLookupResp | Hit/miss plus qualified DATA handle and metadata | proposed |
| tag_install_ack | TagInstallAck | Install/replacement result | proposed |
| tag_invalidate_ack | TagInvalidateAck | Detach result and released TAG lease | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Tag array/Table
- Qualified DATA slot/generation handles
- Valid/replacement/coherence metadata
- TAG-owned DATA leases

## Required capabilities to verify

- Set-associative Table/Array lookup
- Replacement arbitration
- Atomic mapping detach/install and lease messages
- PYC/RTL state lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- TAG stores handles only, not payload
- Replacement detaches mapping without freeing referenced DATA
- Stale DATA generation never hits
- Blocked responses do not mutate replacement state

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze cache sets/ways, tag/coherence fields, replacement algorithm, and lookup port count.

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

- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:877` — Tag slot owns mapping; detach does not destroy referenced data slot.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:546` — TAG holds qualified handles only.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
