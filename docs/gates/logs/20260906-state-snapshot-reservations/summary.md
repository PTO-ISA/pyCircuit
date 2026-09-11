# State snapshot reservation gate

Decision 0202 replaces conservative frontend self-writes with analyzer-derived
snapshot proof and a true snapshot/write runtime conflict contract.

## Evidence

- `ACDataFlowAnalyzer` derives exact static/dynamic top-level TableGet indices
  from conditional-effect SSA presence.
- `ac.state.snapshot` uses enum-typed index kind and remains compiler-owned.
- Closure verification recomputes and rejects missing, extra, or forged proof.
- QueueGraph preserves snapshot records separately from writes and transaction
  resources.
- gfsim snapshot readers are mutually compatible; overlapping writers conflict;
  disjoint indices proceed; reservation-only owners never commit.
- Python infers read-only scalar state through lexical state-prefix binding.
- Reusable ROB completion no longer contains `epoch = epoch`. Its plan writes
  only entries while reserving the exact entry and scalar epoch snapshots.

## Gates

- ACIR lit: 160/160 passed.
- `ACDataFlowAnalyzerTest`: 3/3 passed.
- Native C++ suites: 15/15 passed, including CodeGen 105/105.
- Python frontend/public API: 91/91 passed.
- Queue integration: 23 passed, 1 optional skipped.
- Reusable ROB focused integration passes both class-reuse and per-tick
  scan/incremental gates.
- ROB exact counters: 1769/182 Work, 215 activation, 511 closure.
- ISQ baseline remains 952/129 Work, 100 activation, 146 closure.

## Remaining boundary

Candidate/output predicate snapshots and match/choose exact index sets remain
open. The compiler rejects nested all-table conditional-effect snapshots in
this slice; an all-table ISQ bridge must not be treated as final efficiency
evidence without starvation and continuous-update stress gates.
