# Agentic Circuit Table allocation evidence

- Decision: 0156
- Contract epoch: `0.4`
- Scope: scalar allocation frontend, Frozen ACIR mode verification,
  QueueGraph preservation, typed gfsim commit ordering, direct/native C++
  generation, focused Issue Table and ROB examples

## Passing focused lanes

- Python frontend, contract, and Table E2E: 75 tests passed, 1 skipped because
  its optional external lane was unavailable.
- `ACIROpsTests`: 1845 tests passed.
- `GfsimTests --gtest_filter='QueueBlocksTest.*Table*'`: 13 tests passed.
- `GfsimTests --gtest_filter='QueueBlocksTest.*Slot*'`: 3 tests passed,
  including epoch-aware release used to hold allocation input while no free
  Issue Table entry is selectable.
- `CodeGenTests`: 86 tests passed; the focused
  `QueueGraphPlanTest.*` rerun passed 24 tests.
- `acir-opt components/agentic-circuit/test/ACIR/table.mlir`: passed parser,
  printer, and verifier validation.
- Both allocation examples preserved exactly one `replace` writer through
  canonical QueueGraph JSON, compiled with both direct and native typed C++
  generators, and retained the `unsupported provisional Table` PYC rejection.
- LLVM 22 clang-format dry-run passed for the touched C++ sources.

## Local gaps

- `check-acir`/lit was not reported as validated because this host lacks the
  required `FileCheck`, `split-file`, `not`, and `count` tools.
- The changed-file pre-commit wrapper was unavailable (`pre-commit` and the
  toolchain Python `pre_commit` module are not installed); direct LLVM 22
  clang-format and `git diff --check` checks were used for the owned sources.
- Verilog and whole-repository pyCircuit closure were intentionally not run;
  Decision 0156 remains within the provisional typed-gfsim-only Table boundary.
