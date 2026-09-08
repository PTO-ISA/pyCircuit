# Issue #48 DavinciOO IEX implementation review

The in-tree I1, I2, and WBA sources preserve the external
`hengliao1972/DavinciOO@b81ecfc2b634d41886b01fd8724905eeb5bb6551`
behavioral structure while replacing parallel identity layouts with one
nominal `IssueIdentity` and one `IssueAttemptKey`.

The port found four reusable framework defects. Each was reduced to a generic
regression and merged before the dependent design continued:

- PR #55: Python boolean `or` in a pure invariant;
- PR #56: unique SSA names for an invariant after an outer expression;
- PR #57: zero-initialized persistent nominal enum state;
- PR #58 and PR #59: immutable local capture in `ac.find` and exact captured
  types in nested QueueGraph expression verification.

`valid_operand_source` now owns constant-zero, physical identity,
speculative-producer, stage-mask, destination agreement, and producer FlowKey
shape. I1 and I2 call it explicitly. Temporal active/outstanding/retry,
release, tombstone, cancel, apply, drain, and backpressure conditions remain in
their rules.

The mechanical comparison counts are I1 130 to 9, I2 284 to 41, and WBA 262 to
90. No expanded `IssueAttemptKey` leaf chain remains. The required I2 ceiling
of 80 is met with 39 expressions of margin.

The current design tests prove source closure, raw/Frozen ACIR, QueueGraph
planning, generated gfsim C++ compilation, empty-state execution for I1/I2/WBA,
and explicit stateful PYC rejection. They do not prove the complete concrete
token, backpressure, cancel/race, retry, drain, and isolated-instance behavior
matrix. The cards and issue remain open for that execution evidence.

An independent review found that I2 initially used the updated `active` value
to control its operand Ack and that I1 had changed the baseline grant/cancel
order. The final source uses an explicit pre-state `was_inactive` predicate and
restores read-decision-first lexical order. It also removes premature behavior
checklist completion claims. The remaining conservative same-value tombstone
write is recorded against #25 because removing it requires the planned
same-owner implication/arbitration proof; it is not presented as behavior
closure.
