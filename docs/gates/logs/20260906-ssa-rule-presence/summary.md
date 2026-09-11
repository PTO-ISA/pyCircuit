# SSA rule presence gate

Decision 0200 carries the current single-condition rule predicate as exact SSA
presence on state proposals and outputs through Rule, Firing, and QueueGraph.

## Evidence

- Product source, tests, and current documentation expose one `ac.var` concept
  with no alternate long-form public primitive.
- The public compiler analysis is `ACDataFlowAnalyzer`. MLIR's generic
  `DataFlowSolver` is contained only in its private implementation.
- Guarded output and Table proposals use the same verified `!ac.var<i1>` value.
- Unconditional rules synthesize constant-true presence at block entry, where
  it dominates every authored proposal.
- QueueGraph serializes ordered output presence and per-write presence, and its
  verifier rejects disagreement with the current total guard.
- ACIR lit passes 160/160 tests.
- All native C++ suites pass 15/15 tests.
- Python frontend and public API tests pass 90/90 tests.
- Queue integration passes 23 tests with one optional skip. Its lockstep tests
  retain exact counters: ROB 1769/182 Work, 215 activation, 511 closure; ISQ
  952/129 Work, 100 activation, 146 closure.

## Scope boundary

This slice covers one functional condition and zero or one selected output.
Independent effect presence, consume-only false paths, multiple selected
outputs, general CFG joins, pairwise conflict classes, and explicit contender
arbitration remain follow-up work.
