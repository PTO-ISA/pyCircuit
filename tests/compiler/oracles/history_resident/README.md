# Historical resident queue oracles

This directory owns independent expected results for the two-instance reusable
four-entry ROB and oldest-ready ISQ. It contains no DUT implementation and is
never imported by capture, lowering, code generation, or Runtime. The only
historical inputs it reads are the inert hash-checked assets under
`tests/compiler/lit/Source/Inputs/history-resident/baseline`. It does not execute
the retired compilation routes.

`models.py` applies the historical algorithms and explicit old-Q grants.
`generate.py` reproduces the complete host programs in
`test_reusable_rob_scan_and_activation_match_every_tick` and
`test_reusable_oldest_ready_isq_closes_lost_wakeup_and_backpressure` (the latter
contains the `scan_work=1377` receipt). `vectors.json` is the literal handoff;
`test_models.py` checks it against the independent model. The original programs
execute 61 ROB and 81 ISQ epochs; each retains the original 256-epoch finite
bound. Supplemental reset-reachable reservation programs execute 38 ROB and
49 ISQ epochs. These are additional scenarios, not shortened replacements.

The separate `systems` vector group uses only ordinary offer/take actions on
real queues. Its ROB program is identical to the 61-epoch original. Its ISQ
program is a new 81-epoch regular-clock scenario: a false readiness offer is
published on the held-100 take edge and competes with issue on the following
edge. A later true offer observes the old false bit, so value 300 issues before
200. That changed order is independently expected and does not establish the
historical committed-injection trace. Use checker `--scenario system` for this
separate group; `cases` and `witnesses` remain unchanged.

Every epoch checks the complete committed snapshot:

| Root | Queue contents | Additional state | Packed width |
| --- | --- | --- | --- |
| ROB | All 10 boundary queues, including full payloads | Both instances' 8 scalars and 8 full entries | 974 bits |
| ISQ | All 6 boundary queues, including full payloads | Both instances' 8 full entries and 128 ready bits | 616 bits |

Records use the exact historical field names. ROB records are `index:2`,
`generation:16`, `epoch:16`, `value:16`, `done:1`; ISQ records are `index:2`,
`age:8`, `src0_tag:6`, `src1_tag:6`, `value:16`, `valid:1`; readiness records
are `tag:6`, `ready:1`.

Packing concatenates MSB first. ROB queue order is left flush/allocate/completion,
right flush/allocate/completion, left allocated/retired, right allocated/retired.
Then come left/right scalars (`head:2`, `tail:2`, `count:3`, `epoch:16`) and
left/right entry arrays in index order. ISQ queue order is left request/readiness,
right request/readiness, left issued, right issued. Then come left/right entry
arrays and left/right readiness arrays, each in index order. Each queue contributes
one occupied bit and its complete payload, canonically zero when empty. Invalid
resident entries retain all payload fields and remain part of the snapshot.

Each row distinguishes three views: `before` precedes host actions; `work` includes
any exceptional committed pre-Work readiness injection; `after` follows whole-edge
commit. `offer` is an ordinary host proposal, unavailable to Work until its commit.
`take` observes old output and publishes a pop before Work. Both input and output
queues use depth 1, latency 1 and local occupancy: output transactional
`preparePush` rejects a pending host pop, so a take edge creates a bubble. Using
only `canProposePushWithAdditionalPops` would incorrectly remove that bubble.
The original `commitLeftReadinessForRuleCompetition` operation is represented by
`inject`, distinct from offer. Its two historical clear/set rows are preserved
without retiming. An ordinary closed system cannot express this partial-tree
pre-Work operation; the generated module's external boundary harness must own
that environment operation. Regular queue-wrapper system coverage remains
separate and must not be reported as executing those original injection rows.

ROB grants preserve recovery priority; allocation's identity epoch write excludes
completion; a matching completion writes only the done field and excludes same-head
retirement; a stale completion has no state write and can cofire same-head
retirement. Recovery retains entries/tail, and completion has no occupied check.
ISQ readiness update wins its writer grant, dispatch uses the old first free slot,
and issue reads both source tags of all four old entries, including invalid rows.
Accepted dispatch excludes issue; a readiness write excludes issue only when its
tag lies in that exact snapshot set. An unrelated writer can cofire issue.

The supplemental programs discriminate stale same-head completion vs whole-entry
locking, matching same-head exclusion, other-slot cofire, allocation/completion
exclusion, unrelated ready-writer progress, invalid-entry src1 reservations,
dispatch/issue exclusion, and no same-edge issued-slot reuse. The old stress
program is retained; by itself its offer/take timing does not guarantee a writer
and issue actually compete in the same Work epoch. The supplemental committed
injection witness makes that competition explicit.

Run the independent checks:

```sh
.venv/bin/python tests/compiler/oracles/history_resident/test_models.py -v
.venv/bin/python tests/compiler/oracles/history_resident/generate.py --output /tmp/history-resident-golden.json
```

The checker consumes observed JSON `{"rows": [{"epoch": 0, "after": <full
snapshot>}, ...]}` or an equivalent decimal `packed_after` in each row. It rejects
missing/extra epochs and every changed field. Optional `before`, `work`, `taken`,
`grants`, and `writes` are checked when supplied; expected grant metadata cannot
stand in for actual state observations.

```sh
.venv/bin/python tests/compiler/oracles/history_resident/check.py --kind rob --observed /tmp/rob-observed.json
.venv/bin/python tests/compiler/oracles/history_resident/check.py --kind isq --scenario reservations --observed /tmp/isq-witness-observed.json
```

Passing the model's self-checks establishes expected-data consistency and mutation
discrimination. It does not establish generated C++/RTL behavior, gate integration,
full nightly coverage, historical work-invocation counts, or exact backend commit
timeline instrumentation. The existing API/nightly owner must run the emitted DUT
and feed actual complete observations to this checker.
