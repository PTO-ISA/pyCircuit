# ROB and replay: consolidated PR 79 evidence

Final runtime/frontend: `2ba0c5b8`, incorporating upstream `b69cbd7d`.
CI script correction: `9ddb1c72`. The three historical stage directories retain
separate conclusions; this report is the final evidence entrypoint for Decision
0228 and preserved ROB transaction/scheduling Decisions 0176/0177/0189/0194/0196.
All native tools were built from this checkout with `$pyc` (Clang 22/Python 3.11).

## Final results

| Gate | Result | Retained output |
| --- | --- | --- |
| gfsim C++ | 281 passed | native-summary.log |
| Codegen C++ | 136 passed | codegen-summary.log |
| Focused lit | 8 passed | lit-final.log |
| Frontend on b69cbd7d merge | 252 passed, 4 existing skips | frontend-latest.log |
| Contracts | 42 passed | contracts-latest-final.log |
| Full Queue codegen | 30 passed, 2 existing failures, 1 fixture skip | queues-final.log |
| Final ROB and host backpressure | 4 passed | rob-latest.log |
| Typed owner-write transaction | 1 passed | typed.log |
| Producer journal/native projection | 4 passed | producer.log |
| Independent viewer | 4 passed | viewer-final.log |
| Chromium interaction | Passed, no JS errors/network requests | browser.log |
| Full PR pre-commit after CI correction | Passed, including evidence scripts | ci-fix-pre-commit.log |

C++ output files retain original terminal summaries rather than full test traces.
Other final checks passed: 90 pyCircuit unit tests; 6 xbar topology/gfsim cases;
one multi-output and one aggregate/invariant gfsim case; catalog, API hygiene,
IR coverage and documentation build. CLI ran 53 cases successfully; its remaining
installation case passed when pointed at the current build instead of the stale
hard-coded build path. These detailed logs are archived locally, with exact
commands retained in [commands.md](commands.md).

## Exact replay and scheduling evidence

[comparison-latest.json](comparison-latest.json) and
[compare-latest.py](compare-latest.py) preserve the final comparison: single ROB
70 boundaries/188 events, dual 33/62, scan and activation each 61/203. Queue/Table
initial/final state, every commit, normalized topology and every event/token ID
match the pre-refactor recording baseline. Private component snapshots were
explicitly excluded, consistent with the narrowed observation contract.
Counters remain `scan_work=1769 incremental_work=182 activation=215 closure=511`.
The retained [ROB counter output](../20260907-circular-rob-fix/rob-counters.stdout)
and unchanged executable assertions substantiate these counts.

Browser checks cover compact Queue cells, single-value registers, click details,
back/forward reconstruction, atomic animation, pause, drag, zoom and a delayed
Queue fixture. The browser log's final five-boundary counter is the latency
fixture; ROB has 70 boundaries. HTML and screenshots are disposable artifacts.

## Failures, corrections and limits

- The two Queue structural failures remain unresolved: ISQ snapshot-set count
  expected 2, actual 0; same-owner policy count expected 1, actual 3. Assertions
  were not relaxed. [Baseline output](../20260907-circular-rob-fix/baseline-non-rob.log)
  shows they predate replay. Missing external DavinciOO fixture remains a skip.
- The full [decision report](decision_status_report.json) retains 35 missing
  historical evidence paths for Decisions 0176–0210. It is the final curated-tree
  report, not a replacement for earlier stages' claimed results. No new missing
  path or fabricated successful status is accepted.
- Initial migration failures used stale acir-opt-internal/acir-queue-plan;
  rebuilding fixed them. An exploratory PYC run using an unrebuilt tool is not
  evidence of a PYC regression or pass. PYC/RTL/Verilator/release closure is out of scope.
- A concurrent git-add changed status during the contract read-only test; its
  isolated full rerun passed. Validation now avoids concurrent index mutations.
- [First PR CI](https://github.com/PTO-ISA/pyCircuit/actions/runs/34197998847)
  failed Ruff/Black on three evidence scripts. Local pre-commit had incorrectly
  excluded evidence files. Fix 9ddb1c72 formatted/bound locals/used strict zip;
  all comparison outputs remained identical. [Both G0 jobs subsequently passed](https://github.com/PTO-ISA/pyCircuit/actions/runs/34198384667).
- Recording excludes complete private state, reset and nested-aggregate replay.
  The viewer remains independently packaged; env.md and generated artifacts are untracked.

## Retention and provenance

Related targeted stages share this final decision report and command index;
release runs are not consolidated under this exception. Earlier raw files remain
recoverable from commit `9ddb1c72`; an exact local backup precedes deletions at
`.pycircuit_out/evidence-curation/backup-9ddb1c72/`. Existing main evidence is untouched.
Stage versions remain distinguishable: ROB c0b42d91, initial recording 24a891fe,
observer checkpoint f5045ecf, final migration 2ba0c5b8. Replay's historical local
Decision 0223 was renumbered 0227 then 0228 to preserve upstream decisions.

## Curation verification

No runtime or test expectations changed. Final comparison JSON is identical;
contracts 42/42, full-PR pre-commit and strict docs build pass. All deleted-path
references were checked; strict status retains exactly the same 35 missing
paths. [curation-checks.log](curation-checks.log) retains these results.

| Evidence size | Before | After |
| --- | ---: | ---: |
| files | 105 | 30 |
| lines | 9250 | 2726 |
| bytes | 573885 | 171697 |
