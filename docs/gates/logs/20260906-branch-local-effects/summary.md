# Branch-local state-effect gate

Decision 0207 lowers one ordinary Python `if/else` to complementary SSA
presence over distinct lexical state owners and one atomic runtime candidate.

## Evidence

- The frontend emits one branch predicate, one typed Boolean complement, and
  per-assignment presence while keeping input acceptance constant true.
- Rule, Firing, schedule resolution, and QueueGraph independently accept only
  one shared presence or one structurally complementary pair.
- Storage selection preserves two optional writes in one Firing. Generated
  gfsim prepares only the selected owner and publishes it with the input.
- Same-owner branch writes and cross-branch value dependencies fail closed
  until explicit value/state joins exist.
- The public example contains no Queue/Table/source/sink/readiness/atomic
  syntax. Native execution proves false updates only left and true updates only
  right without clearing or rewriting the unselected owner.

## Gates

- Focused frontend branch tests: 2/2 passed.
- Focused branch integration: passed.
- ACIR lit: 163/163 passed.
- Native C++ suites: 15/15 passed, including CodeGen 105/105.
- Python frontend/public API: 96/96 passed.
- Queue integration: 24 passed, 1 optional skipped.
- Agentic Circuit repository contracts: passed.
- ROB counters remain 1769/182/215/511; ISQ remains 1377/200/162/238.

## Remaining boundary

Scalar same-owner branch joins are closed by Decision 0208. Indexed joins,
branch-local outputs, nested/multi-block CFG, multi-input branches, and combined
blocking/discard paths remain open.
