# Circular ROB regression repair

Base checkout: `0e014154285ab3b74c1297a71204e17753f6f337`.
Environment: `/home/lc/.codex/skills/pyc/scripts/run.sh`, current checkout's
`.pycircuit_out/local-clang22/build`, Clang 22 and fixed Python 3.11.
The legacy `dev-llvm22` paths used by the Queue tests are symlinks into that same
build. No artifacts were imported from another checkout.

## Root cause and repair

The first incorrect representation is raw frontend ACIR, before MLIR storage
selection, QueueGraph, C++ generation or runtime arbitration. The recent serial
SSA rebinding path flattened a trailing blocking `if`, processed its body, then
rewrote the saved condition using the final local-version environment.

For allocation, `count != 4` consequently compared `count + 1` with four. For
retirement, `count != 0` compared `count - 1` with zero. The later compiler stages
faithfully preserved these incorrect operands. The raw before/after diffs in
this directory show the condition moving to the branch-entry SSA value.

The repair captures the blocking condition in a fresh compiler-owned local at
the original `if` position, before flattening its body. The existing source-order
SSA path preserves that value. Capture naming avoids user identifiers. Body
expressions still see preceding scalar assignments, including `mirror = count`
after changing count. This restores Decisions 0176, 0177, 0189, 0194 and 0196 under
the serial SSA contract of Decision 0221. Conditional-effect/snapshot contracts
0201, 0202, 0205 and 0206 remain intact.

No runtime/backend change or new verifier restriction is appropriate: both old
and new expressions are legal typed SSA; MLIR cannot reconstruct which Python
source environment was intended. The new lit regression checks the raw operand,
its preservation through the existing MLIR passes, and generated C++ behavior.

## Before and after

- Single ROB: at tick 13, count/tail were 3/3 and request value 40 was retained.
  It now reaches 4/0, retains the fifth request, wraps, retires in order, rejects
  stale generation, and recovers after flush.
- Dual ROB: at tick 24, both completion inputs were empty and entries had done=1,
  but both counts remained one and neither retired. Both now retire 100/200 and
  preserve subsequent instance isolation.
- Equivalence: both schedulers reached tick 35 with the same state and done=1 on
  the right entry; the result wait timed out. Exit 6 did not diagnose activation
  divergence. The full matched-boundary scenario now completes.
- Performance assertions are unchanged: `scan_work=1769 incremental_work=182
  activation=215 closure=511` (also in `rob-counters.stdout`).

All three harnesses now dump tick, named input/output Queues, scalar state, and
entry index/generation/epoch/value/done on failure. Equivalence distinguishes
step failure, epoch mismatch, committed-state mismatch, timeline mismatch and
result timeout. `PYC_ROB_ARTIFACT_DIR` retains generated artifacts and simulation
stdout/stderr without altering stimuli or expected behavior.

## Validation

| Gate | Result |
| --- | --- |
| Current-checkout native tools and GfsimTests build | Up to date |
| Three ROB tests, plus existing host-result backpressure test | 4 passed |
| New generic frontend regression | Five subcases fail before, all pass after |
| Focused lit, including new generated C++ execution | 6 passed |
| Gfsim QueueBlocksTest | 73 passed |
| Typed owner-write-batch transaction | 1 passed |
| Full Queue codegen file | 30 passed, 2 existing failures, 1 skipped |
| Agentic Python frontend | 225 passed, 4 skipped (229 total) |
| Agentic contracts | 42 passed after regenerating the lit coverage ledger |
| Agentic CLI | 53 passed |
| API hygiene, clang-format dry run and git diff --check | Passed |
| Changed-file pre-commit | Unavailable: executable absent in fixed environment |
| Strict decision status | Failed: 35 existing missing historical evidence entries |

The two non-ROB failures were independently reproduced with the HEAD frontend
Python source and the same current-checkout native tools (`baseline-non-rob.log`):
ISQ expects two occurrences of `snapshot_set_1_0 |= ` but gets zero; branch-join
expects one generated write-policy occurrence but gets three. Neither assertion
was relaxed. The skipped Queue case lacks the DavinciOO reference trace fixture.
The frontend's four existing skips are retained. No Verilator or DavinciOO design
implementation lane is claimed.

The decision report has no missing decisions, unverified statuses, deferred
items, or placeholder evidence; its failure is exclusively absent historical
paths for Decisions 0176–0210. Historical statuses/evidence were not fabricated
or redirected to this narrower run. The pre-commit tool is unavailable; C++
formatting and whitespace checks ran, but the complete hook suite is not claimed.

## Evidence and artifacts

`commands.txt` records commands; `.log` files preserve combined stdout/stderr
as captured. `summary.json` records counts/durations and validation gaps.
`decision_status_report.json` is the unmodified strict checker report.

Raw ACIR is retained in `.pycircuit_out/rob-regression/raw-before/` and
`raw-after/`. Final frozen ACIR, QueueGraph JSON, generated C++, harnesses,
executables and simulation output are under `.pycircuit_out/rob-regression/final/`.
The generic lit artifacts are under
`.pycircuit_out/local-clang22/build/compiler/acir/tests/mlir/Transforms/Output/`.
Only bounded diagnostics and raw-IR diffs are archived here.
