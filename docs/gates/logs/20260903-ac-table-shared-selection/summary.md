# Decision 0155 focused validation

Decision 0155 makes authored Table match/choose values shared across endpoint
policies and evaluates each result lazily once per complete gfsim Epoch. The
Agentic Circuit contract remains at epoch `0.4` and PYC/RTL Table lowering
remains provisional and unsupported.

## Environment

- Checkout: current `/home/lc/pyCircuit` worktree
- Python: `/home/lc/opt/agentic-circuit-toolchain/python-env/bin/python`
- C++: `/home/lc/opt/gcc14/bin/c++`
- LLVM/MLIR: `/home/lc/opt/llvm-22.1.8`
- Build directory: `.pycircuit_out/local-gcc14-llvm22/build`
- Build parallelism: 32 jobs after observing 192 CPUs, about 1.4 TiB available
  memory, and host load between roughly 15 and 33

## Passing evidence

- Built `acir-opt`, `acir-queue-plan`, `acir-queue-cxxgen`, `ACIROpsTests`,
  `GfsimTests`, and `CodeGenTests` from the current checkout.
- All 1845 `ACIROpsTests`, all 227 `GfsimTests`, and all 24 focused
  `QueueGraphPlanTest` cases passed.
- 48 focused Python frontend and Table E2E tests passed with all native tool
  paths supplied. The six Table E2E tests also passed independently with the
  local `acir-queue-pycgen`, including the provisional PYC rejection boundary.
- Frontend tests prove one dominating match/choose definition and endpoint SSA
  captures. The checked-in Issue Queue ACIR contains three distinct authored
  matches and one selection; its grant read and valid-clear patch reference the
  same selection values.
- `acir-opt` parsed and verified the checked-in example and round-tripped an
  empty key region for `policy="first"`. Focused malformed snippets produced
  the expected cross-Table choose-mask and arbitrary external-capture errors.
- A `policy="first"` variant also completed ACIR verification, QueueGraph plan
  extraction, native C++ generation, and standalone C++ compilation with an
  empty key-expression list.
- QueueGraph JSON contains one `table_matches`/`table_selections` record per
  shared definition and only `table_match_ref` / `table_selection_*_ref`
  endpoint expressions. Both direct and native generated C++ compiled and ran
  the multi-writer Issue Queue fixture successfully.
- The new gfsim call-count test proves repeated consumers reuse match and key
  results within one Epoch, advance triggers one new evaluation, and reset
  invalidates both caches.
- The pinned LLVM 22 formatter accepts all changed C++ sources, and
  `git diff --check` passes.

## Local gaps

- `check-acir` was not run because this host lacks `FileCheck`, `split-file`,
  `not`, and `count`. The affected parser/verifier inputs were exercised
  directly with `acir-opt`.
- Verilog lanes were not run because Verilator is unavailable and provisional
  Table remains intentionally unsupported by PYC/RTL.
- Black, Ruff, and pre-commit are not installed in the documented Agentic
  Circuit Python environment. Python modules passed `py_compile` and the
  focused unittest lane.
- The repository-wide clang-format command reports pre-existing violations in
  unrelated board, compiler, and generated testbench sources. The exact C++
  files changed here pass the pinned formatter check.
- Whole-repository pyCircuit closure was intentionally not run for this
  Agentic Circuit-only provisional Table change.
