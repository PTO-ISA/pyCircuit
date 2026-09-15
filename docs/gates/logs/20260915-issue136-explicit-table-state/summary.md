# Issue 136 explicit Table state gate summary

Date: 2026-09-15

Branch: `feat/explicit-table-state`

Base commit: `84146e9b`

Decision: 0263

## Result

The issue 136 semantic slice is implemented and verified. Indexed persistent
state is explicit `ac.table` state, selection is receiver-owned
`table.find(...)`, and the former persistent-list and `ac.find` forms fail with
the documented migration diagnostics.

## Passing evidence

- Agentic Python frontend: 395 passed, 2 skipped.
- Queue frontend/codegen E2E: 32 passed, 3 skipped.
- Focused Table array-update parity: 1 passed across generated GFSim, PYC C++,
  and Verilog.
- Focused same-owner atomic transaction/backpressure case: 1 passed.
- New MLIR regressions for lexical priority and module struct state lowering:
  2 passed.
- AC G2 closure passed. Its recorded cases include direct/native GFSim, PYC
  C++ and Verilog parity, bounded Table admission, writer arbitration,
  field/replace ordering, round-robin selection, reset and atomic transactions.
- Changed-file pre-commit, API hygiene, diagnostic catalog, IR coverage ledger,
  strict MkDocs, and strict decision-status validation passed.

## Semantic coverage

- Static and dynamic Table reads/writes, field proposals, dynamic index
  single-evaluation, early return, outputless rules, branch joins, multiple
  owners and ordered same-owner proposals.
- First and unsigned-min selection with stable flattened-index tie breaking,
  lazy value reads, 128-entry masks, and rank-two runtime row views.
- Cross-Table predicate/key reads with separate read-only ownership, activation,
  snapshot and transaction-resource accounting.
- Queue backpressure, conflict cancellation, reset, atomic publication and
  generated-backend cycle parity.
- Stable rejection of persistent list declarations, removed `ac.find`, invalid
  receivers/call shapes/keys/rows/index widths, while static Python collections
  remain elaboration-only.

## Repository baseline boundaries

The full G0/G1 wrapper was attempted before the successful G2 continuation.
It remains unable to report a repository-wide pass for issues outside this
change:

- repository contract checks reject the pre-existing top-level `designs/`
  directory;
- the full `check-acir` run reports 234 passed, 2 unsupported and 9 failures in
  existing hierarchy, memory, process-state, source-provenance, freeze-topology
  and verify-model fixtures;
- CTest passes 16 of 18 suites; the two failing suites contain pre-existing
  exact operation-registry count and process-state `func.call` fixture failures;
- CLI installation/model-plan tests retain existing local aarch64 SDK and
  build-layout failures.

No failing baseline file above is modified by issue 136. The focused ACIR
regressions and the complete applicable G2 backend matrix are green.
