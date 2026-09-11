# Exact choose-key snapshot-set gate

Decision 0204 gives foreign persistent-state reads in a Table choose key exact
evaluation provenance without adding frontend markers or another state scan.

## Evidence

- `ACDataFlowAnalyzer` binds match-predicate reads to the match mask and
  choose-key reads to the canonical choose index result.
- `ac.state.snapshot_set` verifies that a choose source is its index result,
  belongs to the same Rule/Firing, and reads the reserved target in its key.
- Foreign Table reads in shared non-transactional choose keys fail closed.
- QueueGraph accepts only `table_match` or `table_choose_index` set sources and
  verifies their region-local target read.
- Generated gfsim tests the candidate bit, accumulates the foreign index mask,
  and evaluates the key in the same choose loop. Paired index/valid results
  reuse that evaluation and no `snapshot_entry` loop is generated.
- Normal Python `ac.find(..., key=lambda entry: priorities[entry.tag])` lowers
  through `ac.var.choose` and `ac.var.read_element`; Python does not spell the
  snapshot or Table representation.

## Gates

- ACIR lit: 162/162 passed.
- Focused positive/negative snapshot-set lit: 2/2 passed.
- `ACDataFlowAnalyzerTest`: 4/4 passed.
- Native C++ suites: 15/15 passed, including CodeGen 105/105.
- Python frontend/public API: 92/92 passed.
- Queue integration: 23 passed, 1 optional skipped.
- Agentic Circuit repository contracts: passed.
- Existing ROB/ISQ behavior and ISQ counters 1377/200/162/238 are unchanged.

## Remaining boundary

Field-qualified snapshots are closed by Decision 0205. Exact per-index/field
relations, general CFG joins, and representations beyond 64 entries remain
open. Shared non-transactional choose keys intentionally cannot read Table
state until they gain an equivalent verified snapshot closure.
