# Issue #63 OPT-01 invariant composition

Decision 0225 admits one exact-typed, pure, named invariant call from another
invariant in the same deterministic source closure. The frontend constructs the
complete call graph before validation and rejects self/indirect recursion with
the full qualified chain. Each call remains a nested, verifier-visible
`ac.var.invariant` with hygienic predicate SSA.

Call resolution accepts only an unshadowed bare name bound to the captured
invariant definition. Attribute receivers, invariant parameters, and
module/rule parameters or locals with the same spelling take the ordinary
fail-closed call path. A three-level diamond fixture expands one shared leaf
through two callers and proves every predicate argument and SSA definition is
unique.

ACIR allows that nested invariant as the only region-bearing predicate
operation, independently verifies its nominal input, bool result, capture and
effect boundary, and rejects any repeated ancestor name. The shared value
contract pass now expands explicit leaves before callers until no invariant
remains, instead of depending on reverse walk order. QueueGraph, gfsim and PYC
receive only the existing scalarized predicate operations.

The verifier also checks that the invariant yield value is defined in its own
predicate region. Direct and nested external-yield captures fail with a stable
diagnostic. `inlineInvariant()` also uses a checked mapping lookup as a defensive
error path rather than relying on an assertion.

DavinciOO now defines `valid_producer_identity(LoadProducerToken)` once and
calls it from `valid_operand_source(OperandSourceDescriptor)`. A generated
contract-probe matrix executes valid constant-zero, absent, ordinary and
speculative sources plus sixteen malformed cases covering each zero-source
field, ordinary architectural-index bounds, absent-source shape,
non-speculative producer/mask shape, speculative producer validity, destination
identity, mask and each Flow-bearing producer component.

This change improves source composition without claiming fewer dynamic
comparisons. The shared contract changes from one 43-line invariant to two
invariants totaling 47 lines; source AST comparisons remain 15. The current
contract probe contains 21 raw comparisons and 60 scalar comparisons after
aggregate expansion; its two raw invariant operations lower to zero.

The explicit `ac.popcount` composition fixture is part of the generated PYC
coverage ledger; the repository inventory remains an exact 41-operation,
2-type ODS/ledger match. The new external-capture negative is recorded in the
generated ACIR coverage ledger. The exact required Agentic CI command sequence
passes 42 contract tests, 252 frontend tests with five expected skips, and six
CLI tests.
