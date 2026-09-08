# Queue/Table observer refactor — 2026-09-08

Compared against pre-replay ROB repair `c0b42d91` and recording baseline
`24a891fe`, on `feat/gfsim-replay`. Decision 0223 is refined to separate
synchronous state notifications, model-owned registration, external payload
codecs, and the recording adapter. Decisions 0176, 0177, 0189, 0194 and 0196
retain their functional and scheduling contracts.

Only registered Queue/Table state is exported. Private buffers, cursor, memory
storage and sink histories are intentionally removed. No MLIR semantic change;
no backend-specific functional fix. Viewer remains an independent uncommitted
package under third_party/circuit-flow-viewer. env.md is untouched.

## Results

| Check | Result |
| --- | --- |
| Current-checkout Clang 22 native build | Pass |
| gfsim C++ | 276 passed (5 additional observer/lifecycle regressions) |
| Recording producer | 4 passed |
| Full Queue codegen | 30 passed, 2 existing failures, 1 existing skip |
| Single/dual/scan-activation ROB and host backpressure | All passed within full Queue file |
| Baseline record comparison | 70 single, 33 dual, 61 scan and 61 activation boundaries identical for Queue/Table state and events |
| Exact events compared | 188 single, 62 dual, 203 scan, 203 activation |
| Scheduling counters | 1769 scan Work, 182 activation Work, 215 activation edges, 511 closure edges; unchanged |
| Typed state transactions | 1 passed |
| Focused lit | 4 passed, including external codec and registration metadata |
| Frontend | 225 passed, 4 existing skips; final focused codegen 13 passed |
| Contracts | 42 passed after final documentation changes |
| CLI run / inventory | 7 / 1 passed |
| Independent viewer unit / Chromium | 4 passed / pass, no page errors |
| API hygiene / changed-line C++ format / new C++ file format / diff whitespace | Pass |

Common-state codegen validation now actually instantiates observation
registration, starts and finishes sessions for nine native generated component
families, plus direct Python-generated broadcast, feedback and memory models.
The existing functional assertions are retained. Native observer tests cover
same-row disjoint writers' intermediate merge results, exception context cleanup,
opaque payload execution without a codec, failed attachment isolation, and
continued execution/system reset after finish. Existing tests retain atomic
multi-owner backpressure/retry and equal-valued Queue identity through latency.

Comparison normalizes singular/plural connection keys, then compares identities,
names, paths, kinds and connections. It compares every Queue/Table before/after
image and exact event dictionaries (including sequence, token and operation IDs).
Removed private-state fields are excluded explicitly, not relaxed arbitrarily.
The comparison script is adjacent; baseline traces remain in
`.pycircuit_out/replay/verified-rob`, final traces in
`.pycircuit_out/replay/refactor/verified`.

## Invasiveness and disabled-recording footprint

Against pre-replay `c0b42d91`, the five runtime files retain only 81 inserted and
4 removed lines: dispatch context, one observer pointer, Queue/Table read-only
views and synchronous notifications, and system boundaries/reset checking.
`queue_blocks.h` differs only in SimTable; other primitive implementations and
private payload types are back to their pre-replay form. These headers and
system.cpp contain no replay/Replay references or file-encoding dependency.
Generated model registration is template-based and allocates nothing until an
observer session is requested. External payload codecs replace member methods.

| Object (aarch64, unsigned payload) | Before replay | Initial replay | Refactored |
| --- | ---: | ---: | ---: |
| SimObject | 120 B | 128 B | 128 B |
| SimQueue | 312 B | 416 B | 320 B |
| SimTable | 256 B | 264 B | 264 B |

Each Queue loses its four always-present metadata vector objects (96 bytes).
The remaining 8 bytes over the original baseline are the common observer pointer.
This is not zero overhead: disabled execution retains pointer checks and the
scope wrapper; enabled execution allocates adapter state and snapshots.

A local -O2 microbenchmark performs 2,000,000 enqueue/dequeue pairs without
recording. Seven-run median times were 64.063 / 69.589 / 64.383 ms respectively,
with identical checksums. The raw measurements and source are adjacent. This is
an unisolated local diagnostic, not a performance gate or general throughput
claim; the real ROB scheduler counters and behavior assertions remain unchanged.
Historical header sources were extracted from this repository's Git objects;
all benchmark binaries were built locally using the same fixed compiler.

## Remaining gaps and scope

- The same pre-existing Queue text assertions fail: ISQ snapshot variable count
  (expected 2, actual 0), and same-owner branch-join policy count (expected 1,
  actual 3). The DavinciOO external fixture remains unavailable. No expectations
  were changed to obtain the above results.
- Strict decision-status still fails only on 35 historical missing evidence
  references for Decisions 0176–0210. The report is adjacent; no evidence was
  invented and Decision 0223's new references exist.
- pre_commit is absent from the fixed environment; direct format and hygiene
  checks passed. No full pre-commit, documentation build, release matrix,
  Verilator or DavinciOO design validation is claimed.
- Queue/Table registration is explicit; private component state, nested-entry
  visualization, reset replay, field-level causes and cross-barrier private
  buffer lineage remain outside this observation contract.
- Chromium required sandbox escalation for IPC. The independent viewer code was
  not modified by this refactor; its existing Queue/Register/Table presentation
  consumed the new records successfully. Its screenshots are under
  third_party/circuit-flow-viewer/evidence/observer-refactor.
- No commit or remote push was performed for the refactor. The user-requested
  checkpoint is 24a891fe; third_party/circuit-flow-viewer and env.md remain outside
  Git staging.

New review page: `.pycircuit_out/replay/refactor/viewer/replay.html`.

