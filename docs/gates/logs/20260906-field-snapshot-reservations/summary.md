# Field-qualified snapshot reservation gate

Decision 0205 propagates direct Entry field reads through analyzer proof,
QueueGraph, generated plans, and gfsim conflict checks.

## Evidence

- `ACDataFlowAnalyzer` carries `ac.var.get` field selection back to each
  contributing Table snapshot; complete values expand in declaration order.
- `ac.state.snapshot` and `ac.state.snapshot_set` require verifier-checked
  `read_fields`, and closure independently compares those fields.
- QueueGraph preserves and validates reservation fields against Entry payloads.
- Generated structured and flat models use allocation-free exact relation
  clauses, indexed by `entry * field_count + field`.
- gfsim distinguishes whole-entry, replace, and field-merge conflicts. Same
  entry/disjoint field merges proceed; overlapping fields and replacements
  conflict for pending and prepared writers. Heterogeneous two-entry/two-field
  coverage proves cross writes remain independent.
- Reusable ROB completion reserves only `entries.{generation, epoch}` plus the
  scalar epoch owner. Its Python remains free of state/transaction markers.

## Gates

- ACIR lit: 162/162 passed.
- Focused snapshot/rule lit: 4/4 passed.
- `ACDataFlowAnalyzerTest`: 4/4 passed.
- Focused gfsim snapshot tests: 2/2 passed.
- Native C++ suites: 15/15 passed, including CodeGen 105/105.
- Python frontend/public API: 92/92 passed.
- Queue integration: 23 passed, 1 optional skipped.
- Agentic Circuit repository contracts: passed.
- ROB counters remain 1769/182/215/511; ISQ remains 1377/200/162/238.

## Remaining boundary

Partial Entry/field relations currently require
`entries * declared_fields <= 64`. General CFG joins and wider exact relation
representations remain open.
