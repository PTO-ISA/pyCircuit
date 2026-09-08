# TMU.BGF.XBAR — Crossbar

- Source candidate: `DAV-TMU-BGF-XBAR-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `BGF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/bgf/xbar.py`
- Current design-program execution status: **implemented with static egress
  generation; gfsim behavior and PYC C++/Verilog build verified; H2/H1
  integration remains pending**.

Existing registered transport leaf; it routes complete BGFPacket transactions and marks destination delivery only.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| ingress_0 | BGFPacket | Independent requester ingress | declared |
| ingress_1 | BGFPacket | Independent requester ingress | declared |
| ingress_2 | BGFPacket | Independent requester ingress | declared |
| ingress_3 | BGFPacket | Independent requester ingress | declared |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| delivered | BGFPacket | Packet after selected destination-bank delivery; completed and delivered_bank are set | declared |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Internal merge, route, destination latency, and completion Queue stages only

## Required capabilities to verify

- Already supported merge/route/apply hierarchy
- Production replacement must connect grants and BANK acknowledgements rather than treating pilot latency as a bank

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- FIFO within each ingress and round-robin arbitration across ingresses
- Retain full packet under output backpressure
- Delivery marker is transport completion, never Tile publication or PTO commit

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Decide whether the existing pilot remains the production XBAR or is replaced by an ARB-grant-driven transport leaf.

## Focused implementation evidence

- `designs/davincioo/tmu/bgf/xbar.py` freezes four ingress ports, round-robin
  ingress/completion merges, destination routing, and static latencies
  `(1, 2, 4, 7)` from one indexed `apply` template.
- `pytest -q designs/davincioo/tests/fabric/test_xbar_static_generation.py`
  checks the frozen packet schema, exact Queue topology, all four destinations,
  latency offsets, full-packet identity, per-ingress FIFO under contention,
  backpressure retention, exactly-once completion, in-flight reset, active
  instance isolation, and PYC C++/Verilog build closure.
- The static template depends on [Decision 0227](../../../../docs/rfcs/pyc6-decisions.md#decision-0227-static-queue-collections-elaborate-supported-operator-families).
  Framework and design gate results are archived under
  `docs/gates/logs/20260908-issue63-opt06-static-queue-array/` and
  `docs/gates/logs/20260908-issue63-opt06-davincioo-xbars/`.

## Contributor closure

- [x] Claim the candidate and identify its parent/containing state owner.
- [x] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [x] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [x] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [x] Implement the accepted owner and design-local expected-result tests.
- [x] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `srcs/core/tmu/bgf/xbar.py:28` — Declared four Queue inputs and one Queue output.
- `docs/specification/davincioo/ndf-next/interfaces/modules.md:22` — Normative ingress and completion boundaries.
- `docs/specification/davincioo/ndf-next/tile/tmu.md:69` — BGF transport must preserve identity and not report architectural completion.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
