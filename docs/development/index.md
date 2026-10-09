# Development guide

Development targets the approved source-unit pipeline: capture each Python
source, resolve published interfaces and verify the linked final design in
MLIR, then emit C++ or Verilog from that same artifact. Current support and
remaining language limitations are documented independently of historical
migration completion.

## Current contract and workflow

- [Repository map](repository-layout.md): folder owners and where changes belong.
- [Source-unit workflow](source-unit-workflow.md): current compiler stages and public flow.
- [Agent frontend guide](agent-frontend-guide.md): source decomposition and
  ownership rules.
- [Language reference](../reference/language.md): typed modules, ordinary
  variables, rules, structs/tables, Enum, branches and rejection boundaries.
- [Current limitations](known-limitations.md):
  post-Enum/match priorities across Python, MLIR, codegen, runner and examples.
- [Contributing workflow](contributing-workflow.md): change process and scope.
- [Testing and gates](testing-and-gates.md): candidate-bound validation.
- [Review and merge](review-and-merge.md): semantic and evidence review.

## Toolchain profiles

CompilerDev requires exact LLVM and MLIR 22.1.8. Runtime-only consumers use
`find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)` and
`pycircuit::pyc6_runtime` without LLVM discovery. Generated model builds expose
source-owned `pycircuit_modules`; host drivers use typed `pyc_dut` and the shared
SystemRunner. Closed `@system` designs generate executable simulation harnesses;
a shared-library port C ABI remains outside the current supported contract.

## Project governance

[Project governance](project-governance.md) describes bounded ownership,
independent review and validation. Use descriptive responsibility names in
active code and docs. Local migration packets and logs are ignored. Frozen
approval proposals remain records of contract authority, not runtime evidence.

## Historical records

Retired frontend implementations and their user guides remain in Git history.
Frozen contracts and approval records under `docs/rfcs/contracts/` preserve
decision provenance; they are not current build instructions. The
[verification evidence policy](../gates/README.md) distinguishes local records
from checked-in source and reproducible tests.
