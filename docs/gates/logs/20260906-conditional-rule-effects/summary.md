# Conditional rule effects gate

Decision 0201 separates token acceptance from state-effect presence for the
first outputless, one-input early-return rule slice.

## Evidence

- Python uses one ordinary early `return`; no Queue, Table, marker, ready/full,
  pop/push, sink, or transaction primitive is exposed.
- Raw ACIR carries conditional presence on generic `ac.var.assign_element`, and
  storage selection preserves it on `ac.table.propose`.
- `ACDataFlowAnalyzer` and typed footprints distinguish the constant-true input
  candidate from predicate-qualified state writes.
- Rule/Firing and QueueGraph verification reject non-i1, non-implying,
  zero-input, and multiple-predicate conditional effects.
- Generated gfsim distinguishes a stalled `nullopt` candidate from an engaged
  input-only plan. Absent writes publish no Table proposal or commit.
- Snapshot reservations remain independent of writes: overlapping lexical
  writers force re-evaluation, while disjoint indices proceed.
- The reusable ROB completion rule uses this path. Same-epoch allocation and
  completion remain serializable; stale completion commits its input without an
  epoch or entry Table commit.

## Gates

- ACIR lit: 160/160 passed.
- Native C++ suites: 15/15 passed, including CodeGen 105/105.
- Python frontend/public API: 90/90 passed.
- Queue integration: 23 passed, 1 optional skipped.
- ROB exact counters are superseded by Decision 0202's removal of the fake
  epoch write: 1769/182 Work, 215 activation, 511 closure.
- ISQ exact counters: 952/129 Work, 100 activation, 146 closure.

## Remaining boundary

Reservation indices currently follow potential writes. Exact predicate
read-set reservations must be derived by `ACDataFlowAnalyzer` before dummy
self-writes can be removed generally. Optional outputs, general CFG joins,
conflict classes, and explicit contender arbitration remain open.
