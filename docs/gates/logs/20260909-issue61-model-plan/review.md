# Independent architect review

## Verdict

APPROVE — 0 unresolved findings.

Architectural status: `CLEAR`.

## Reviewed contracts

- `model plan` requires one explicit SDK root and binds the running launcher,
  Python package, native extension, QueueGraph planner, and plan schema to the
  same closed platform manifest before generation.
- The exact platform, distribution, ABI, capability, path, size, and hash tuple
  fails closed. Native extension bytes are checked before import, rechecked
  immediately before loading, and the loaded path is verified afterward.
- Consumer Python executes only inside the bounded isolated capture worker.
  Importable module identity preserves relative imports, and source/config
  identities are checked before and after capture.
- Planning reuses QueueGraph JIT, native ACIR freeze, and the installed
  QueueGraph planner. It does not add a second semantic lowering path.
- Canonical plan artifacts are root and hash-seed independent except for the
  local depfile. The public examples and SDK checker use the same fixed
  `model.h`, `model.cpp`, and `queuegraph.cpp` output tuple.
- Initial publication uses atomic directory rename. Replacement holds a sibling
  lock and uses Linux `renameat2(RENAME_EXCHANGE)` or macOS
  `renamex_np(RENAME_SWAP)`, so the five-file plan directory changes as one
  name operation.
- Public failures use the frozen `ACSDK-PLAN-*` diagnostic family. The generic
  diagnostic schema and runtime validator accept the same family.
- Python 3.11 installed tests cover relative imports, schema-valid output,
  repeat publication, directory exchange, wrong SDK identity fields, wrong
  root/platform/ABI, tampered launcher/native/schema/tool, missing tools,
  source/config TOCTOU, bounded capture timeout, hash seeds, and unsafe output
  paths. Python 3.14 is rejected as an SDK profile while the remaining CLI
  regression suite stays green.

## Evidence

The evidence records the complete Python 3.11 build and installed-prefix
assembly commands. A real installed invocation produced `model-plan.json`, and
`check-sdk-contract.py --document` validated that exact runtime document.
Repository contracts, SDK contracts, 90 unit tests, 58 compatible CLI tests,
strict documentation, decision-status validation, and pre-commit passed.

Decision 0233 correctly remains deferred because P3 still owns `emit-cpp`,
generated runtime ABI implementation, output cleanup, and compile/link
integration.
