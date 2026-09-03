# Decision 0154 focused validation

Decision 0154 adds field-disjoint scalar and masked Table writers while keeping
the Agentic Circuit contract epoch at `0.4` and preserving the provisional PYC
rejection boundary.

## Environment

- Checkout: current `/home/lc/pyCircuit` worktree
- Python: `/home/lc/opt/agentic-circuit-toolchain/python-env/bin/python`
- C++: `/home/lc/opt/gcc14/bin/c++`
- LLVM/MLIR: `/home/lc/opt/llvm-22.1.8`
- Build directory: `.pycircuit_out/local-gcc14-llvm22/build`
- Build parallelism: 48 jobs after inspecting 192 CPUs, about 1.4 TiB available
  memory, unrestricted cgroup CPU/memory, and host load around 40

## Passing evidence

- Built `acir-opt`, `ACIROpsTests`, `GfsimTests`, `CodeGenTests`,
  `acir-queue-cxxgen`, `acir-queue-plan`, and `acir-queue-pycgen` from the
  current checkout.
- 55 focused Python frontend, Queue codegen, and Table E2E tests passed. The
  native plan/PYC-boundary case was then run with all local tool paths supplied
  and passed.
- The `tests/e2e/fixtures/table_examples/table_multi_writer_issue.py` fixture compiled and
  executed through both direct and native QueueGraph C++ paths. Its two masked
  writers merged `src0_ready=true` and `src1_ready=true` on the same Entry;
  old-state selection then observed the ready Entry on the following tick and
  committed `valid=false`. A grant-driven read delivered that selected old
  Entry to the sink in the same tick as the clear proposal.
- After promoting the fixture to a public example, all six tests in
  `tests/e2e/test_table_backend.py` passed with the configured local native
  tools.
- The example's two independent match regions exposed and fixed duplicate
  top-level SSA names in frontend lowering. All 42 Queue frontend tests passed
  after assigning each masked endpoint a stable, unique expression prefix.
- 11 Table-focused gfsim tests passed, including same-Entry field merge,
  different-Entry proposals, masked updates, disabled writes, old-state reads,
  next-tick visibility, and writer-local cancellation.
- 24 QueueGraph plan/codegen tests passed. Canonical JSON assertions cover
  `write_fields` on the Table endpoint and runtime block.
- `table.mlir` parsed, verified, printed, reparsed, and preserved `$entry` plus
  struct field lists. Focused malformed inputs produced the expected missing,
  empty, duplicate, unknown, and cross-writer-overlap diagnostics.
- The canonical Frozen ACIR for the public example is checked in as
  `examples/state/table_multi_writer_issue.mlir`; it passed `acir-opt`
  canonical round-trip, QueueGraph planning, and native C++ generation.
- The native Table E2E test confirmed PYC still rejects the family with
  `unsupported provisional Table`.
- Earlier in this run, the complete `ACIROpsTests` (1845 tests), `GfsimTests`
  (224 tests), and `CodeGenTests` (86 tests) binaries passed after the core
  implementation; final focused tests passed after formatting and test/docs
  additions.

## Local gaps

- `check-acir` was not run because this host lacks the required LLVM
  `FileCheck`, `split-file`, `not`, and `count` tools documented in `AGENTS.md`.
  The Table parser/verifier cases were exercised directly with `acir-opt`.
- Verilog lanes were not run because Verilator is unavailable and provisional
  Table remains intentionally unsupported by PYC/RTL.
- The changed-file `pre-commit` command could not run because the
  `pre-commit` executable is not installed on this host. C++ sources were
  formatted with the pinned LLVM 22 formatter, Python files passed
  `py_compile`, and `git diff --check` passed.
- A direct API-hygiene scan of the Agentic Circuit documentation reports five
  pre-existing `PYC415` matches on legacy Agentic Queue collection selection
  examples. Those are not pyCircuit Wire method calls and were not introduced
  or changed by Decision 0154.
- Whole-repository pyCircuit closure was intentionally not run for this
  Agentic Circuit-only provisional Table change.
