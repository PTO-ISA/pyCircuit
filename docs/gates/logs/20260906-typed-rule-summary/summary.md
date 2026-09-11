# Typed rule summary bridge gate

Decision 0199 replaces string-only analysis vocabulary for the current
single-condition subset with independently verified closed typed summaries.

## Evidence

- Lowering derives typed Queue checks, effects, output-presence kind, state
  accesses, guard/schedule kind, and arbitration membership.
- The Firing verifier reconstructs the exact records from endpoints, condition,
  footprints, proposals, and priority.
- Tampering a predicate guard summary to always is rejected.
- ACIR lit passes 160/160 tests.
- The complete ROB and ISQ lockstep tests retain identical committed state and
  timelines with unchanged exact counters: ROB 1769/182 Work and ISQ 952/129
  Work for scan/incremental execution.

## Scope boundary

This is a typed-summary bridge only. Guard/presence categories are not SSA path
identities, state access kinds are not conflict classes, and arbitration
membership is not a contender graph. Those remain explicit plan items.
