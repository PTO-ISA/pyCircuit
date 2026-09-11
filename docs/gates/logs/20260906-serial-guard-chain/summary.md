# Serial early-return guard-chain gate

Decision 0206 lets outputless one-input rules express multiple readable stale
or discard checks with ordinary serial Python early returns.

## Evidence

- The frontend combines contiguous pre-effect returns into one SSA conjunction
  of inverted conditions and keeps the rule candidate constant true.
- All following generic `ac.var` assignments share that effect presence;
  storage selection and MLIR dataflow derive snapshots and atomic closure.
- Non-contiguous guard returns and state effects before the chain fail closed.
- Reusable ROB completion uses separate generation and epoch returns without
  exposing Queue, Table, readiness, reservation, or commit syntax.
- Generated gfsim retains one Work object and the existing grouped
  prepare/publish/no-fail commit path.

## Gates

- Focused frontend guard-chain tests: 4/4 passed.
- Reusable ROB focused integration: passed.
- Python frontend/public API: 94/94 passed.
- ACIR lit: 162/162 passed.
- Native C++ suites: 15/15 passed, including CodeGen 105/105.
- Queue integration: 23 passed, 1 optional skipped.
- Agentic Circuit repository contracts: passed.
- ROB counters remain 1769/182/215/511; ISQ remains 1377/200/162/238.

## Remaining boundary

Distinct-owner branch-local state effects and one `else` are closed by Decision
0207. Same-owner joins, nested CFG, optional/multiple selected outputs, and
multi-input discard paths remain open. These shapes must carry explicit SSA
path proof rather than being flattened syntactically.
