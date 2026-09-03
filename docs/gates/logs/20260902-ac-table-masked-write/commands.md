# Decision 0153 focused gate commands

Environment: the repository-local GCC 14, LLVM 22, CMake, and Python paths from
`AGENTS.md`. Host capacity was inspected and `PYC_LOCAL_BUILD_JOBS=64` was used.

- Built `acir-opt`, `acir-queue-plan`, `acir-queue-cxxgen`,
  `acir-queue-pycgen`, `acir-opcode-catalog`, `ACIROpsTests`, `GfsimTests`, and
  `CodeGenTests` from the current checkout.
- Ran queue frontend and public API unittests (49 passed).
- Ran contract and IR coverage unittests (36 passed).
- Regenerated `schemas/opcodes.json` with `acir-opcode-catalog` and
  `docs/spec/50-verification/ir-coverage.md` with
  `scripts/check-ir-coverage.py --write-ledger`.
- CTest reported `No tests were found`; ran the three C++ binaries directly:
  1845 + 224 + 86 tests passed.
- Ran `tests/e2e/test_table_backend.py` with explicit current-build paths for
  opt, plan, native C++ generation, and PYC generation (5 passed), including
  the minimal batch-wakeup example.
- Parsed and compiled the uniform complete-Entry masked write fixture through
  both direct and native C++ generators.
- Ran `acir-opt` directly on the positive Table IR and the extracted
  masked-owner negative case.
- Ran Python `py_compile`, changed-C++ LLVM 22 format dry-run, the exact
  repository-wide LLVM 22 format command, and `git diff --check`.
- Ran normal and strict decision-status validation after recording this
  evidence.

The repository-wide formatter failed only on existing out-of-scope files. The
host lacks FileCheck, split-file, not, count, black, and ruff; no unavailable
lane is claimed.
