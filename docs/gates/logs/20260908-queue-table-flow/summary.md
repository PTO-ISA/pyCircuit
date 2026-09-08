# Initial Queue/Table recording — 24a891fe

Based on ROB repair `c0b42d91`. This stage introduced opt-in committed Queue/Table
snapshots and successful operation records in PYC6TRC3. Its local Decision 0223
was later renumbered to 0228; it is not upstream's multi-output Decision 0223.
The HTML viewer was independent and untracked at this stage; it was subsequently
committed under `third_party/circuit-flow-viewer` in `f5045ecf`.

Validation used current-checkout Clang 22/Python 3.11: 271 gfsim C++ tests,
4 producer tests, 4 ROB/backpressure tests, 1 typed transaction, 4 focused lit,
225 frontend tests (4 skips), 42 contracts and 8 focused CLI tests passed.
The 61-boundary scan/activation records each contained 203 data events and
matched exactly. Counters remained 1769 / 182 / 215 / 511.

Producer tests independently compared native Queue/Table projections with the
journal and covered atomic retry/backpressure, equal-valued token identities,
delayed readiness and simultaneous operations. Viewer unit and Chromium checks
passed. Coverage excludes private component state, nested-entry visualization,
reset replay and PYC/RTL support.

The two pre-existing Queue structural failures, missing external fixture, and
35 historical decision evidence gaps persisted. Pre-commit and docs builds were
not available/claimed at this stage. Final checks do not retroactively change
these historical outcomes.

Final evidence and reproduction commands are centralized in
[the migration report](../20260908-replay-main-migration/summary.md) and
[commands](../20260908-replay-main-migration/commands.md). The shared
[decision report](../20260908-replay-main-migration/decision_status_report.json)
checks the final curated tree, not this historical stage. Earlier reports and
repeated logs were archived locally before curation; they remain recoverable
from pre-curation commit `9ddb1c72`.
