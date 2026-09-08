# Observer separation — checkpoint f5045ecf

Compared with ROB repair `c0b42d91` and initial recording `24a891fe`.
Queue/Table retain synchronous observation hooks; the external adapter owns
snapshots, codecs, proposal metadata and token IDs. Model assembly registers
state/topology. Ordinary primitives no longer export private state. Local
Decision 0223 is now 0228; transaction/scheduling contracts are unchanged.

At this stage: 276 gfsim tests, 4 producer tests, 4 focused lit, 1 typed
transaction, 225 frontend tests (4 skips), 42 contracts and 8 focused CLI tests
passed. Native registration was instantiated for nine component families;
direct Python broadcast/feedback/memory registration also compiled and ran.
Viewer unit/Chromium checks passed. Full Queue codegen retained two structural
failures and one fixture skip; strict status retained 35 historical missing paths.
Complete pre-commit/docs builds were not claimed at this stage.

Record comparison preserved Queue/Table projections, topology and exact events:
single 70 boundaries/188 events, dual 33/62, scan and activation 61/203 each.
Removed private-state snapshots were explicitly outside the comparison.
Final-version parity is independently retained in the shared migration evidence.

## Disabled-recording footprint

| Object, unsigned payload on aarch64 | Before replay | Initial replay | Refactored |
| --- | ---: | ---: | ---: |
| SimObject | 120 B | 128 B | 128 B |
| SimQueue | 312 B | 416 B | 320 B |
| SimTable | 256 B | 264 B | 264 B |

Four permanent Queue metadata vectors were removed (96 bytes). One observer
pointer and disabled-path checks remain; this is not zero overhead.
`footprint.cpp` and `footprint.json` retain the source and seven-run measurements
for 2,000,000 enqueue/dequeue pairs: medians 64.063 / 69.589 / 64.383 ms,
identical checksum. This unisolated local diagnostic is not a throughput gate.
Header sources came from this repository's Git objects, with all binaries built
locally by the same compiler. ROB scheduler counters remained unchanged.

Final evidence and reproduction commands are centralized in
[the migration report](../20260908-replay-main-migration/summary.md) and
[commands](../20260908-replay-main-migration/commands.md). The shared
[decision report](../20260908-replay-main-migration/decision_status_report.json)
checks the final curated tree, not this historical stage. Earlier reports and
repeated logs were archived locally before curation; they remain recoverable
from pre-curation commit `9ddb1c72`.
