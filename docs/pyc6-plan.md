# pyCircuit plan status

This page replaces the superseded CycleAwareSignal and Agentic Circuit delivery
plan. The active product contract is the approved source-unit scalar profile
consolidated in [Decision 0283](rfcs/pyc6-decisions.md#decision-0283-approved-source-unit-hardware-cutover-for-the-scalar-profile).
M5 is accepted for that profile on 2026-10-01; see the [final review](reviews/20261001-m5-cutover-review.md).

## Current route

The product path captures each Python source independently, resolves published
source interfaces, links the explicit unit closure into a verified final
hardware design, then emits C++ or Verilog from that saved artifact. The public
commands are `pycircuit compile`, `pycircuit link`, and `pycircuit emit`.

The supported authoring profile is portless function `@module`, nested
`@rule`, explicit registration, one default clock, finite scalar state, and
empty static arguments. Reads see current state for the whole epoch; `nonlocal`
assignments propose next state; the system Xfer stage commits state. There are
no queues, builder/JIT semantics, or eager-Python simulation fallback.

Runtime-only consumers use the installed `pycircuit::pyc6_runtime` target
without LLVM. CompilerDev requires LLVM/MLIR 22.1.8. Generated model CMake
provides source-owned translation units, `pycircuit_system`, and
`libpycircuit_dut`.

## Acceptance boundary

M5 acceptance evidence covers installed
compile/link/emit/build/run, both backends from one saved final artifact,
source ownership and maps, invalid-publication preservation, Runtime-only and
CompilerDev consumers, and static plus dynamic retirement of old routes. See
the [M5 work item](work-items/m5-cutover.md) and
[retirement ledger](work-items/m5-retirement-ledger.md); historical gate runs
do not satisfy current acceptance by themselves.

## Later capabilities

The scalar profile does not deliver a complete `@system`/EXPECT contract,
queues and buffer libraries, memory, CDC, multiple clocks, four-state source
values, external typed DUT ports, complex static parameters, or parallel
scheduling. These remain explicit capability work with independent semantic
oracles. The [modernization plan](development/pycircuit-modernization-plan.md)
tracks phase sequencing; it does not widen the currently approved profile.

## M6 progress

[M6-01](work-items/m6-publication-relocation.md) is accepted on 2026-10-01:
real-process publication termination/recovery and complete moved-prefix use on
macOS arm64. Production semantics and interfaces remain the M5 baseline.
[M6-02](work-items/m6-incremental-scale.md) is also accepted: measured
shared-definition 1/16/64 and distinct-source 1/8/32 builds, real Ninja no-op and
invalidation, per-source C++ groups, and literal CPP/RTL oracles. Counter
clean/rebuild, Python 3.12 helper execution and generated RTL CMake space paths
were repaired within C3. Source recompilation is selective; full backend emit
still rebuilds all generated C++ targets.

Broader fault/platform/SDK matrices, backend build optimization and actual
parallel scheduling remain later M6 packets; RSS and throughput are unmeasured.
M7 release remains separate.

## M7 preview progress

[M7-01](work-items/m7-local-preview.md) is accepted on 2026-10-01 for a local
macOS 26/arm64 preview of the unchanged scalar profile. Fresh native/installed
SDK, source-owned semantic closure, earlier M6 gates, current presets and a
disposable wheel all pass with independent review and explicit skip/deselection
limits. The release runbook now records the current local recipe.

No release workflow, public tag/index or publication was dispatched. Existing
`v6.1.0` identifies an older source revision; formal platform/minimum-OS and stable
release acceptance remain separate work. This does not close the wider M3/M6
capability backlog or the entire migration roadmap.

## M3/M6 extension roadmap

The [expanded roadmap](work-items/m3-m6-expansion-plan.md) separates new
capability use cases from existing-contract hardening. The next preparation
lanes are M3-P01 contract/evidence reconciliation and M6-03 first-publication
fault coverage, followed by M6-04 resource/run measurement. Bank static carriers,
SYSTEM/EXPECT B and any new interfaces require their precise approval gates;
this roadmap does not widen the current scalar profile.

## Current extension execution

M3-P01 admission is complete: records require a narrow final-declaration
projection addendum, with existing C1/C2/R1 semantics preserved. The
[next E01 packet](work-items/m3-record-final-projection-design.md) is design-only;
Bank and SYSTEM/EXPECT B have no new implementation approval.
[M6-03](work-items/m6-first-publication-expansion.md) is accepted for macOS/POSIX:
15 new first-publication interruptions plus bundle recovery-before-republish
observations passed. No production protocol or interface changed.

## Record approval and resource baseline

C2-DECL-R revision B is precisely approved; [record implementation slices](work-items/m3-record-implementation.md)
have completed [S1 final declarations](work-items/m3-record-s1-final-declarations.md).
S2 helper/value/use closure is next; record state and executable product support
remain unimplemented. Shared emission stays closed until S4; the three reproduced
baseline SourceMath/APInt failures remain explicit validation debt.
[M6-04](work-items/m6-resource-runtime-baseline.md) closes selected developer
resource/finite-C-ABI measurements on macOS with honest sampled-RSS and timing
limits. Existing scalar runtime/source contracts and all other unapproved
capability boundaries remain unchanged.
