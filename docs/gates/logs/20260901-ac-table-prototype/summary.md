# Decision 0151 provisional Table evidence

Result: pass with declared host-tool gaps.

The contract epoch `0.4` vertical slice is implemented from ACPy through
Frozen ACIR and QueueGraph to typed gfsim C++. The tests cover integer and flat
struct entries, zero/reset state, state-driven and queue-driven reads,
backpressure, disabled read/write behavior, old-data read-during-write,
full writes, field-preserving patch lowering, static and dynamic bounds,
single-writer enforcement, stable identities, and the legacy `ac.memory`
regression.

The provisional boundary remains intentional: QueueGraph-to-PYC rejects a
Table with `ACLOWER-PYC: unsupported provisional Table: PYC lowering is
deferred`. No Table RTL or cross-backend atomicity support is claimed.

## Results

- Full current-checkout build: 178/178 build steps, install and installed-package doctor passed.
- C++ units: 2311 tests passed across 11 executables.
- Python frontend: 116 passed, 1 skipped.
- Contract tests: 36 passed; schema/catalog inventory passed at epoch `0.4`.
- Table native end-to-end: 2 passed; existing memory PYC regression: 1 passed.
- ACIR Table parse/print/parse, IR coverage ledger, API hygiene, and `git diff --check`: passed.
- Substantive Table C++ files pass LLVM 22 `clang-format --dry-run --Werror`.

See `commands.txt`, `gates.stdout`, `gates.stderr`, `summary.json`, and
`decision_status_report.json` in this directory.

## Declared local gaps

This host does not provide FileCheck, split-file, not, count, or Verilator, as
already documented in `AGENTS.md`; therefore full lit/check-acir and Verilog
lanes are not claimed. It also lacks pytest, mkdocs, and pre-commit commands.
The repository-wide formatter command encounters pre-existing unrelated
formatting violations; the substantive Table C++ source set passes the pinned
formatter. These gaps do not expand Decision 0151 beyond its documented
gfsim-only prototype boundary.
