# pyCircuit agent instructions

This repository has completed the pyCircuit M5 hard-break cutover for the
declared scalar profile and is progressing through bounded M6 hardening.
Active product behavior is governed by the approved migration contracts and
the accepted implementation. Do not treat this file or any migration proposal as evidence that
implementation or tests have passed.

## Read first

- `docs/development/agent-frontend-guide.md`
- `docs/reference/language.md`
- `docs/development/contributing-workflow.md`
- `docs/development/testing-and-gates.md`
- `docs/development/review-and-merge.md`
- `docs/development/m5-migration.md`
- `docs/rfcs/migration/approvals/` for exact approved migration contracts
- `docs/development/project-governance.md` and
  `docs/development/pycircuit-modernization-plan.md` for modernization
  ownership and phase expectations

## Active contract

The current bounded source profile is one portless function `@module` per
implementation source, nested `@rule` definitions with explicit module-scope
registration, one default clock, finite scalar state with current/next
semantics, and empty static arguments. Each source compiles independently to a
published unit. Link takes the explicit complete unit closure; C++ and Verilog
emit from the same verified final artifact.

Python capture does not execute the design. MLIR owns source resolution,
semantic checks, interface/effect derivation, and hardware lowering. Rule reads
observe current state throughout the epoch; `nonlocal` writes propose next
state. Use ordinary local candidates when a proposed value is needed again.

Unsupported uses must fail closed with a diagnostic. The current profile does
not include a complete `@system` contract, queues, memory/CDC, multiple clocks,
four-state source values, external typed ports/DUT ABI, dynamic collections, or
nonempty static parameters. Do not fill gaps through retired routes.

## Hard break and build contract

The active product route is `pycircuit compile`, `pycircuit link`, and
`pycircuit emit --target cpp|verilog`. There is no fallback or compatibility
mode for CycleAwareSignal/JIT, structural builders, Agentic Circuit/QueueGraph,
`acc.py`, `acc`, or `pycc`.

The CMake build options are `PYC_BUILD_COMPILER_DEV`, `PYC_BUILD_TESTING`, and
`PYC_BUILD_RUNTIME_LIB`. Runtime-only consumers use
`find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)` and
`pycircuit::pyc6_runtime`; that component must not require LLVM. CompilerDev
requires exact LLVM/MLIR 22.1.8. Generated model CMake builds
`pycircuit_system` and `libpycircuit_dut` against Runtime. Supply a finite
runner configuration; event output is silent by default and requires explicit
`--events`.

Keep complete consumer designs, consumer testbenches, ISA decoders,
model-comparison scripts, consumer schemas, and adapters in their owning
repositories. Framework semantics stay design-neutral.

## Modernization work

Follow `docs/development/project-governance.md` and the modernization plan for
bounded ownership, approved contracts, independent tests/reviews, candidate
identity, and evidence. Do not modify frozen approval proposals or milestone
status as part of implementation/documentation work unless that file is
explicitly assigned.

For any feature or bug fix, identify affected decisions/contracts and the
smallest evidence needed. Build from this checkout; do not copy toolchains,
shared libraries, or generated outputs from another worktree. Keep generated
files and temporary artifacts out of the source tree. Update active behavior
docs with behavior changes.

## Verification and reporting

Use the narrowest relevant gate, then the applicable candidate acceptance
checks. Bind semantic or decision-bearing evidence under
`docs/gates/logs/<run-id>/` to exact candidate content and commands. Report
commands, exit status, evidence paths, skipped checks, and remaining callers.
A fixture or named test is not a passing result. A documentation-only update
must not claim M5 implementation, acceptance, or completion.
