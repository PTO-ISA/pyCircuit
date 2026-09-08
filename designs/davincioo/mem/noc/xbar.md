# MEM.NOC.XBAR — Crossbar

- Source candidate: `DAV-MEM-NOC-XBAR-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `NOC`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/noc/xbar.py`
- Current design-program execution status: **implemented with static egress
  generation; gfsim behavior and PYC C++/Verilog build verified; H2/H1
  integration remains pending**.

Existing registered transport-only leaf with frozen pilot Queue ports and NoCPacket schema.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| ingress_0 | NoCPacket | Independent transaction ingress | declared |
| ingress_1 | NoCPacket | Independent transaction ingress | declared |
| ingress_2 | NoCPacket | Independent transaction ingress | declared |
| ingress_3 | NoCPacket | Independent transaction ingress | declared |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| delivered | NoCPacket | Locally delivered packet with delivered_port and transport_completed set | declared |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Internal merge, route, destination latency, and completion Queue stages only

## Required capabilities to verify

- Already supported merge/route/apply hierarchy
- Production NOC integration may require parameterized ports/VC grants beyond the pilot

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Each ingress is FIFO; arbitration across ingresses is round-robin
- Retain exact packet under destination/output backpressure
- transport_completed means local XBAR delivery only
- No response matching, stale-generation rejection, resource release, instruction completion, or PTO event publication

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Decide how the four-port pilot maps to the selected production router/VC topology.

## Focused implementation evidence

- `designs/davincioo/mem/noc/xbar.py` freezes four NOC ports, round-robin
  ingress/completion merges, destination routing, and static latencies
  `(1, 2, 3, 5)` from one indexed `apply` template.
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

- `srcs/core/mem/noc/xbar.py:38` — Declared four Queue inputs and one Queue output.
- `docs/specification/davincioo/ndf-next/interfaces/modules.md:120` — Normative pilot ingress and transport completion boundary.
- `docs/specification/davincioo/ndf-next/tile/memory.md:58` — Transport leaf excludes transaction-owner responsibilities.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
