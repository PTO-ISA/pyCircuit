# Remaining work before merge

The user stopped further adaptation on 2026-10-07 and requested immediate draft
PR submission. This list is not a completion claim.

- Adapt the remaining 61 retained examples to source `@system` with generated
  C++/Verilator simulation, preserving original behavior and independent oracles.
- Reconcile all 93 original design roots, including 12 retained API cases and
  23 unfinished historical items; removed rows do not count as verified.
- Finish bare `@system`, `@rule`, `@module` spelling across examples and docs.
- Finish API/system test registration and run final clean-checkout CI, formatting,
  documentation and packaging checks before merge.
- Verify source-import IR, transformed IR and both backend artifacts per root.
- Run deferred nightly coverage, reference/mutation matrices and platform/release
  checks through the existing entrypoints.
- Fix the concrete language/runtime limitations in
  [known limitations](known-limitations.md), including direct fixed-type imports,
  generated namespace collisions, wide/four-state observations and RTL diagnostics.

Closed-system generation has focused independent execution and failure-atomicity
coverage. Full example adaptation and full release validation are incomplete.
