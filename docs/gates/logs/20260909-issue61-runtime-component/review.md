# Independent architect review

## Verdict

APPROVE — 0 unresolved findings.

Architectural status: `CLEAR`.

## Reviewed contracts

- `AgenticCircuit::Gfsim` and the Runtime export are free of LLVM, MLIR,
  ACIRBindings, and GfsimTooling dependencies.
- LLVM-backed trace I/O and run-manifest support are isolated in
  `AgenticCircuit::GfsimTooling` and the separate tooling include root.
- Compiler tools link Tooling, Gfsim, ACIRBindings, and the LLVM closure in
  static-library dependency order.
- Explicit Runtime and CompilerDev components, the default CompilerDev
  behavior, missing default CompilerDev CLI, hidden LLVM/MLIR packages, and
  unknown components have positive or fail-closed evidence.
- Installed targets and headers contain no source-tree or build-tree paths.
- Decision 0232 correctly remains deferred until the remaining issue #61
  package, relocation, and release work is complete.

## Review iteration

The first review found that the implicit default CompilerDev component did not
set `AgenticCircuit_FIND_REQUIRED_CompilerDev`. A damaged installation could
therefore satisfy `find_package(AgenticCircuit REQUIRED)` while its CLI was
missing. The final change sets the required flag and adds an installed negative
regression that renames the CLI and verifies configuration failure.

Fresh final evidence records 285 Gfsim tests, 143 CodeGen tests, four
installation tests, 90 unit tests, the focused MLIR compile test, component
consumer checks, repository contracts, SDK contracts, strict documentation,
decision-status validation, and pre-commit.
