# Exact ISQ snapshot-set gate

Decision 0203 extends analyzer-derived state reservations to candidate/output
presence and to the exact foreign-state indices evaluated by one Table match.

## Naming contract

- `ac.var` is the sole ACIR variable family; no `ac.variable` or Python
  variable constructor exists.
- Passes consume the public `ACDataFlowAnalyzer`; MLIR's generic solver is
  confined to the analyzer's private implementation.

## Evidence

- `ACDataFlowAnalyzer` roots snapshot discovery at candidate, output, and state
  presence.
- `ac.state.snapshot_set` binds a target Table and the actual source match mask.
  Local and closure verifiers independently validate its provenance.
- QueueGraph preserves the dependency as `index_kind = "set"` with its source
  SSA identity, separate from writes and transaction resources.
- Structured and flat gfsim codegen accumulate a `uint64_t` mask during the
  original match scan. Generated code contains no second `snapshot_entry` scan.
- The reusable ISQ reserves all scanned entries and only the two source-tag
  readiness indices evaluated for each entry. Repeated placements still share
  one implementation class and own independent state.
- Same-tag clear/write conflicts force re-evaluation. Continuous unrelated
  readiness writes do not starve issue; repeated source tags and tag 63 are
  covered.

## Gates

- ACIR lit: 161/161 passed.
- Focused snapshot-set lit: 1/1 passed.
- `ACDataFlowAnalyzerTest`: 3/3 passed.
- Native C++ suites: 15/15 passed, including CodeGen 105/105.
- Python frontend/public API: 91/91 passed.
- Queue integration: 23 passed, 1 optional skipped.
- Agentic Circuit repository contracts: passed.
- Expanded ISQ exact counters: 1377 scan Work, 200 incremental Work, 162
  activation traversals, and 238 Work-closure traversals.

## Remaining boundary

Choose-key dependency sets are closed by Decision 0204. Field-level snapshots,
general CFG joins, and snapshot-set representations beyond 64 entries remain
open. They must retain exact evaluation provenance and may not introduce
frontend markers or a second variable concept.
