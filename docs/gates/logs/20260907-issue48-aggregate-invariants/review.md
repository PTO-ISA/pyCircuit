# Issue #48 framework review

Decision 0224 adds exact same-descriptor recursive equality and one explicit
named payload predicate. The public surface remains ordinary typed Python:
`left == right`, `left != right`, and a one-argument `@ac.invariant` function.
No Queue readiness, packing, reservation, or commit mechanism is exposed.

The review closed these correctness gaps before sign-off:

- the existing enum-only ordered-comparison diagnostic remains stable while a
  separate aggregate diagnostic rejects ordered comparison;
- ACIR independently enforces the stable `<Payload>.<function>` invariant name
  and rejects nested predicate regions in addition to effectful operations and
  external SSA captures;
- both rule closure and direct topology freeze reject residual aggregate
  comparison or invariant operations;
- QueueGraph independently rejects forged residual contracts;
- the canonical rule pipeline lowers contracts before value analysis, state
  selection, rule effects, handshake inference, and QueueGraph planning;
- a 104-bit recursive fixture executes equal and one unequal case for each
  nested field family in gfsim, accepts valid invariant inputs, and consumes
  invalid inputs without publishing them;
- an admitted packed fixture produces identical C++ and Verilator cycles and
  results while canonical PYC remains scalar-only.

The framework implementation is merge-ready. Issue #48 remains open after this
PR because its in-tree DavinciOO I1/I2/WBA canonical-identity and comparison
reduction outputs are a serial follow-up against the merged framework revision.
