# Queue/Table flow recording — 2026-09-08

Framework branch: `feat/gfsim-replay`, based on ROB repair `c0b42d91`.
Decision 0223 adds opt-in observation only; related Decisions 0149, 0176, 0177,
0189, 0194 and 0196 retain their semantics.

The framework exports committed Queue/Table state and successful operations.
HTML generation and browser dependencies live in the independent local
`third_party/circuit-flow-viewer` package, intentionally not staged or committed.

Current-checkout Clang 22 build and Python 3.11 are selected by
`/home/lc/.codex/skills/pyc/scripts/run.sh`. CLI tests unset
`AC_GATE_TOOLCHAIN_ROOT` inside that environment because their helper otherwise
loads an older installed native runtime instead of the current build.

Validation results and exact commands are recorded in the adjacent command log.
Full logs and generated ACIR/QueueGraph/C++/traces remain in
`.pycircuit_out/replay/`; only concise reviewable evidence is archived here.

Known baseline: the full Queue codegen file has two pre-existing generated-text
assertion failures (ISQ snapshot name count, branch-join policy count), plus one
missing external trace skip. No ROB assertions or performance counter
expectations were relaxed. Historical missing decision evidence is tracked
separately by strict decision-status output.

## Results

| Check | Result |
| --- | --- |
| Current-checkout native build | Pass |
| gfsim C++ | 271 passed |
| Record format/runtime projections | 4 passed |
| Single ROB, dual ROB, scan/activation, host backpressure | 4 passed |
| Scan/activation recorded projections | 61 boundaries equal; 203 data events in each mode |
| Performance contract | 1769 scan Work / 182 activation Work / 215 activation edges / 511 closure edges, unchanged |
| Full Queue codegen file | 30 passed, 2 baseline failures, 1 baseline missing-trace skip |
| Typed owner-write transaction | 1 passed |
| Focused MLIR/codegen | 4 passed |
| Frontend | 225 passed, 4 existing skips |
| Contracts | 42 passed after regenerating the IR coverage ledger for the new lit case |
| Run CLI and command inventory | 7 + 1 passed |
| API hygiene, changed-line C++ format, new-file C++ format, diff whitespace | Pass |
| Independent viewer unit tests / wheel smoke | 4 passed / pass |
| Chromium offline interaction | Pass, 70 boundaries, 16 objects, zero page errors or network requests |

`ReplayTest` independently writes native Queue values and Table rows after each
boundary without using ReplayValue or recorder snapshots. The Python producer
test compares every reconstructed boundary to that projection. Equal-valued
Queue elements keep distinct identities through delayed readiness and
simultaneous push/pop. Blocked candidates emit no successful data operations;
retry commits Queue consumption/production and multiple owner writes together.
The record-enabled dual ROB test also exercises two simultaneous operations in
one boundary without merging their identities.

Browser review confirmed visible FIFO cells, all four ROB rows, actual modified
fields, read versus write colors, animated connections, atomic state updates,
selection details and reversible navigation. The page uses English UI labels
so the development host's limited font installation does not obscure evidence;
model names and data are unchanged. The viewer is intentionally a v1 offline
in-memory consumer, with no nested-entry/reset/private-state replay claim.

## Remaining validation gaps

- The full Queue file retains the same two pre-existing failures as the ROB-fix
  baseline: `test_reusable_oldest_ready_isq_closes_lost_wakeup_and_backpressure`
  expects an obsolete snapshot variable count, and
  `test_same_owner_branch_join_emits_one_state_proposal` expects one generated
  policy occurrence instead of the current three. Expectations were not changed.
- Strict decision-status validation reports the existing 35 missing evidence
  paths for Decisions 0176–0210. Decision 0223 has concrete existing evidence;
  no new missing path was introduced. Historical evidence was not fabricated.
- `pre_commit` is absent in the fixed Python environment. Changed-line C++
  formatting, whitespace and API hygiene checks were run directly; this does
  not claim the complete pre-commit or documentation-build lane passed.
- No Verilator, DavinciOO design, release matrix, publication, or remote push
  was performed. The independent package and all replay feature changes remain
  unstaged/uncommitted as of this delivery; `env.md` was left untouched.

The delivered local pages are `.pycircuit_out/replay/flow-review/replay.html`,
`dual-rob.html`, and `queue-latency.html`. Reproduction commands and evidence
summaries are adjacent; complete generated files remain in disposable output.
