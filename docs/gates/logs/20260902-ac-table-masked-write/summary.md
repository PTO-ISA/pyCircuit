# Decision 0153 masked Table update evidence

Result: pass with declared host-tool and repository-format gaps.

Contract epoch `0.4` now supports `table.view(mask).write(...)` and
`table.view(mask).patch(...)` for a same-Table `CandidateSet`, while the
existing scalar-index read/write/patch surface is unchanged. The masked
endpoint is state-driven, shares the single-writer limit, evaluates per-Entry
patch lambdas from old committed state, and commits the complete selected set
atomically.

## Results

- Focused frontend and public API: 49 tests passed.
- Contract and IR inventory: 36 tests passed; the opcode catalog and generated
  IR coverage ledger include `ac.table.masked_write`.
- C++ units: `ACIROpsTests` 1845, `GfsimTests` 224, and `CodeGenTests` 86 tests
  passed (2155 total).
- Table end-to-end: 5 tests passed. The masked example compiled and ran through
  both the Python direct generator and native QueueGraph C++ generator; the
  existing scalar Table scoreboard stayed green.
- The minimal batch-wakeup example lowered to a masked Table write and compiled
  through both direct and native C++ generators.
- A uniform complete-Entry masked `write` fixture parsed through `acir-opt` and
  compiled through both direct and native C++ generators.
- The native PYC generator rejected the masked fixture with the stable
  `unsupported provisional Table` boundary.
- Positive masked ACIR parsed and round-tripped; the same-width cross-Table
  negative case produced the stable same-Table ownership diagnostic.
- Changed C++ files pass LLVM 22 clang-format dry-run; `git diff --check` and
  Python bytecode compilation pass.

## Declared gaps

- This build tree registers no matching CTest cases, so the three required C++
  test executables were run directly and their exact test counts are reported.
- FileCheck, split-file, not, and count are unavailable, so `check-acir` is not
  claimed. The new positive file and masked-owner negative split were exercised
  directly with `acir-opt`.
- The required repository-wide LLVM 22 format command remains blocked by 66
  pre-existing files outside this change. The first reported file is
  `boards/zybo_z7_20/ps/linx_monitor/src/linx_platform.h`; other existing
  failures include PYC compiler sources, imported AC references, and Janus test
  sources. No unrelated file was reformatted.
- black and ruff are unavailable in the isolated Python environment.

No PYC/RTL implementation, multidimensional mask, Queue-carried mask, masked
read, multi-writer arbitration, or cross-object transaction is claimed.
