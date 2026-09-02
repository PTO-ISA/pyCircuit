# Decision 0152 focused gate commands

Environment: the local GCC 14 / LLVM 22 / Agentic Circuit toolchain from
`AGENTS.md`, with `PYC_LOCAL_BUILD_JOBS=64` after checking CPU, memory, load,
and cgroup capacity.

- Built `acir-opt`, `acir-queue-plan`, `acir-queue-cxxgen`,
  `acir-queue-pycgen`, `acir-opcode-catalog`, `ACIROpsTests`, `GfsimTests`,
  and `CodeGenTests` with the local CMake/Ninja build tree.
- CTest selection returned `No tests were found`; ran `ACIROpsTests` (1845),
  `GfsimTests` (223), and `CodeGenTests` (86) directly. All passed.
- Ran the focused queue frontend, public API, and Table e2e unittest files:
  51 tests, 1 skipped, no failures.
- Ran contract, IR coverage, and inventory determinism unittest files:
  37 tests, 1 skipped, no failures.
- Ran `scripts/check-ir-coverage.py --write-ledger`, then its read-only form;
  the manifest/ODS/lit coverage check passed.
- Lowered the state example through Frozen ACIR and QueueGraph, generated both
  direct and native C++, compiled both, and ran the same slot no-refill harness;
  both executables passed.
- Ran `acir-queue-pycgen` on the fixture and observed the required
  `unsupported provisional Table: PYC lowering is deferred` rejection.
- Parsed the new match-domain, choose-count, and duplicate-release negative
  split cases individually with `acir-opt`; each produced its checked stable
  diagnostic.
- The changed C++ file set passed LLVM 22 clang-format dry-run and
  `git diff --check` passed. The required repository-wide formatter command was
  also run and failed on existing files outside this change.
- Both normal and strict decision-status validation passed for all 152 rows;
  the normal report is archived beside this summary.

Unavailable host tools were not substituted: FileCheck, split-file, not,
count, black, and ruff. No Verilog, PYC RTL, or whole-pyCircuit gate is claimed.
