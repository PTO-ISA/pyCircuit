# Optional output-presence gate

Decision 0210 gives one stateful rule independent SSA output presence and
predicate-qualified backpressure without exposing Queue checks in Python.

## Evidence

- The frontend emits a constant-true candidate, candidate-qualified state, and
  `ac.rule.output` carrying the ordinary Python condition.
- Rule/Firing verification requires one input and a true candidate for optional
  output presence; typed checks/effects are reconstructed from that presence.
- QueueGraph preserves separate candidate, state-write, and output identities.
- Generated gfsim prepares output capacity only when the optional value exists.
- With the output Queue held full, an absent-output token consumes and increments
  count; a present-output token retains its input and state until capacity is
  released, then commits all effects together.

## Gates

- Focused frontend optional-output test: passed.
- Focused output-backpressure integration: passed.
- ACIR lit: 166/166 passed.
- Native C++ suites: 15/15 passed, including CodeGen 105/105.
- Python frontend/public API: 98/98 passed.
- Queue integration: 27 passed, 1 optional skipped.
- Agentic Circuit repository contracts: passed.
- ROB counters remain 1769/182/215/511; ISQ remains 1377/200/162/238.

## Remaining boundary

Multiple independently selected outputs, pure optional transforms, nested CFG,
multi-input discard/output paths, and combined branch-local effects remain open.
