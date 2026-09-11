# Scalar same-owner branch-join gate

Decision 0208 joins two scalar branch values before storage selection so one
lexical owner retains one proposal and one atomic commit.

## Evidence

- Compiler-owned `ac.var.select` verifies an i1 condition and exact matching
  value/result types; it is not exported as a Python primitive.
- The frontend recognizes one complementary scalar assignment per arm, emits
  one value join, and emits one unconditional generic state assignment.
- QueueGraph preserves a typed three-operand `value_select` and one owner write.
- gfsim emits a C++ ternary; PYC lowering emits `pyc.mux`.
- Native execution observes false-arm incremented value 8 followed by true-arm
  direct value 9 while the frozen plan contains one `total` proposal.

## Gates

- Focused ACIR/PYC/gfsim select lit: 4/4 passed.
- Focused frontend branch tests: 2/2 passed.
- Focused branch integration: 2/2 passed.
- ACIR lit: 165/165 passed.
- Native C++ suites: 15/15 passed, including CodeGen 105/105.
- Python frontend/public API: 96/96 passed.
- Queue integration: 25 passed, 1 optional skipped.
- Agentic Circuit repository contracts: passed.
- ROB counters remain 1769/182/215/511; ISQ remains 1377/200/162/238.

## Remaining boundary

Indexed same-owner joins are closed by Decision 0209. Branch-local selected
outputs, nested/multi-block CFG, multi-input branches, and combined
blocking/discard paths remain open.
