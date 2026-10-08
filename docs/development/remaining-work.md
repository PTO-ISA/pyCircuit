# Remaining work before merge

The current task prioritizes example migration, API repairs and independent
verification. This list records acceptance gaps and is not a completion claim.

- Complete system migration and full verification for the retained examples.
  Regular-clock benches retain their original module drivers and independent
  oracles; their reset, held-clock and four-state scenarios require separate
  verification where generated default-clock systems cannot express them.
- Reconcile all 93 original design roots, including 12 retained API cases and
  23 unfinished historical items; removed rows do not count as verified.
- Finish bare `@system`, `@rule`, `@module` spelling across examples and docs.
- Finish API/system test registration and run final clean-checkout CI, formatting,
  documentation and packaging checks before merge.
- Verify source-import IR, transformed IR and both backend artifacts per root.
- Run deferred nightly coverage, reference/mutation matrices and platform/release
  checks through the existing entrypoints.
- Restore full asserted-module queue vector execution without changing errors
  into stalls or dropping the historical positive/fault scenarios.
- Fix the remaining concrete limitations in
  [known limitations](known-limitations.md), including generated namespace
  collisions, explicit physical domains, wide/four-state observations and RTL
  diagnostics. Direct fixed-type imports and explicit imported Struct expression
  constructors now have independent positive and rejection coverage.

Closed-system generation has focused independent execution and failure-atomicity
coverage. Full example adaptation and full release validation are incomplete.
