# Issue 25 Table writer arbitration closure

## Scope

Decision 0237 is implemented for the Agentic Circuit frontend, ACIR verifier
and typed summaries, QueueGraph planning, and typed gfsim execution.

Same-field writers are accepted only when the compiler proves disjoint fields,
indices, or guards from the committed snapshot, or when every conflicting
endpoint declares explicit owner-local `writer_priority` metadata. Stable
endpoint identities and declared ranks form an acyclic precedence relation;
source, traversal, object, and Work order are not semantic tie-breaks.

The selected winner enters resource preparation. Losing transitions reserve no
Queue, Table, Reg, Slot, or output and publish no effects. Compatible disjoint
writes evaluate from one old Table image and merge into one next image.

## Evidence

- `check-acir`: PASS, 192 of 192 tests.
- Native ACIR/analysis/QueueGraph/gfsim lane: PASS, 4 of 4 tests.
- Python public API and Queue frontend: PASS, 166 of 166 tests.
- Focused Decision 0237 MLIR regression: PASS, all 12 `RUN` directives in one
  lit test. It covers typed metadata, static and guarded disjointness, missing
  policy, duplicate rank and endpoint identity, malformed policy, missing
  identity, safety-assertion rejection, cross-owner precedence cycles, and
  rule-to-firing metadata preservation.
- `git diff --check`: PASS.
- Decision-status validation with concrete existing evidence: PASS.
- Strict MkDocs build: PASS.

Raw stdout, stderr, and return-code files are stored beside this summary. Exact
commands are in `commands.txt`.

## Boundary

This closure does not admit Table into canonical PYC or RTL. Decision 0241
still owns the bounded register-bank Table profile and C++/Verilog parity.
Until that decision lands, the supported boundary remains the stable
`unsupported provisional Table` diagnostic. The current evidence verifies
frontend, ACIR, QueueGraph, and typed gfsim behavior only.

## Result

Decision 0237 is implemented-verified for issue #25. Decisions 0238 through
0241 remain independent deferred work.
