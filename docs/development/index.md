# Development guide

Development targets the approved source-unit pipeline: capture each Python
source, resolve published interfaces and verify the linked final design in
MLIR, then emit C++ or Verilog from that same artifact. M5 candidate verification
is in progress; these pages do not certify the cutover as complete.

## Current contract and workflow

- [M5 migration guide](m5-migration.md): current profile, public commands,
  runtime profiles, and remaining capability/caller work.
- [Agent frontend guide](agent-frontend-guide.md): source decomposition and
  ownership rules.
- [Language reference](../reference/language.md): portless function modules,
  nested rules, current/next state, and rejection boundaries.
- [Contributing workflow](contributing-workflow.md): change process and scope.
- [Testing and gates](testing-and-gates.md): candidate-bound validation.
- [Review and merge](review-and-merge.md): semantic and evidence review.

## Toolchain profiles

CompilerDev requires exact LLVM and MLIR 22.1.8. Runtime-only consumers use
`find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)` and
`pycircuit::pyc6_runtime` without LLVM discovery. Generated model builds expose
source-owned translation units, `pycircuit_system`, and
`libpycircuit_dut`.

## Project governance

[Project governance](project-governance.md) and the
[modernization plan](pycircuit-modernization-plan.md) describe ownership and
milestone sequencing. Decision 0283 records the accepted scalar profile and
states that candidate verification is still active. Frozen approval proposals
remain records of contract authority; they do not provide runtime evidence.

## Legacy documentation

Pages about CycleAwareSignal/cycle balancing, structural builders, Agentic
Circuit/QueueGraph, PYC IR, and sidecar schedules are labeled as retired or
historical. They must not be used as build instructions or support claims.
