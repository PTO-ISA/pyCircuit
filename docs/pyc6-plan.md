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
Broader fault/platform matrices, measured build/scale behavior and actual
parallel scheduling remain later M6 packets; M7 release remains separate.
