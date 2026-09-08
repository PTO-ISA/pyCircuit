# Replay migration to main

Branch: `feat/gfsim-replay`. Pre-merge checkpoint: `f5045ecf`.
Upstream merged: `d6bc7baf` (22 commits since common base `0e014154`).

## Resolution

- Preserve both upstream and local implementation changes. Three Markdown
  conflicts: decision corpus, decision status, generated IR coverage ledger.
- Upstream Decisions 0223–0226 retain their IDs. The local Queue/Table recording
  decision becomes 0227; active references updated. Historical evidence using
  0223 describes the same local feature before the numbering collision.
- Regenerate the coverage ledger from the combined source and tests.
- Track the independent HTML package in `third_party/circuit-flow-viewer`.
  It remains independently packaged, with no runtime dependency on pyCircuit.
  Generated HTML, screenshots, browser dependencies and `env.md` are excluded.
- Keep the local source-position blocking-guard capture alongside upstream #70's
  committed-state guard rewriting. Frontend and ROB gates verify the combination.
- Adapt the nested-payload source assertion to upstream OPT-05's `const auto &`
  aggregate observation. The executable's payload/value assertions are retained;
  no performance counter or functional expectation is relaxed.

## Verified behavior

- gfsim C++: 281 passed. Codegen C++: 136 passed.
- Frontend: 255 tests, 251 passed / 4 existing skips. Contracts: 42 passed.
- pyCircuit unit lane: 90 passed. Focused lit: 8 passed.
- Producer replay: 4 passed. Independent viewer unit: 4 passed.
- Typed state transaction: 1 passed. Upstream multi-output and recursive
  aggregate/invariant gfsim execution: 1 passed each.
- Final Queue codegen file: 33 tests, 30 passed / 2 pre-existing failures /
  1 existing skip. Three ROB regressions and host backpressure pass. Four recordings match the
  pre-refactor baseline exactly: Queue/Table initial/final state, each commit,
  topology and every event including occurrence IDs and operation associations.
  Single: 70 boundaries/188 events; dual: 33/62; scan and activation: 61/203 each.
- ROB counters remain `scan_work=1769 incremental_work=182 activation=215 closure=511`.
- Chromium verified actual migrated ROB HTML: Queue geometry, register display,
  selection/details, backward/forward reconstruction, atomic animation, pause,
  drag, zoom and delayed Queue fixture. No JS errors or network requests.
- CLI full run: 53 passed, one installation test initially used a hard-coded
  build path without CMake install metadata. Rerunning that test with its BUILD
  constant set to the current checkout build passed (1/1); no installed artifacts
  were borrowed from another checkout.
- Documentation build, API hygiene, regenerated IR coverage and changed-source
  pre-commit hooks pass (merge markers, whitespace, Ruff, Black, Markdown).
  Diff whitespace checks pass excluding historical gate logs; raw historical
  logs on both branches contain pre-existing trailing spaces and are retained.

## Diagnostics and scope

- Initial lit failures came from stale `acir-opt-internal`; rebuilding that
  target made all eight selected tests pass. Upstream gfsim tests likewise
  required rebuilding `acir-queue-plan`; final focused results use current tools.
- Strict decision-status still reports the same 35 historical missing evidence
  paths for Decisions 0176–0210. No new missing path is introduced by 0227.
- Queue codegen's two pre-existing structural assertion failures remain:
  ISQ snapshot-set spelling/count (2 versus 0) and same-owner policy occurrence
  count (1 versus 3). Their assertions have not been changed.
- PYC/RTL/Verilator and release closure are outside this Agentic Circuit/gfsim
  migration. An exploratory broad upstream suite also invoked PYC using an
  unrebuilt tool; those exploratory results do not establish a PYC regression
  or a PYC pass. Final upstream checks deliberately target the gfsim cases.
- Full disposable logs and regenerated HTML are under
  `.pycircuit_out/replay/main-migration/`.
