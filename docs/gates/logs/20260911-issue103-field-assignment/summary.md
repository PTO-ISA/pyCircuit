# Issue 103 immutable field-assignment evidence

Decision 0236 permits direct field assignment inside `@ac.rule` as frontend
syntax for immutable record replacement. Local and persistent-scalar targets
normalize to ordinary SSA `with_fields(...)` rebinding. An indexed persistent
target caches its index once and emits the same complete-entry proposal as the
explicit immutable spelling.

The focused frontend regressions compare shorthand and explicit raw ACIR,
cover serial local SSA, persistent scalar and list owners, conditional effects,
local-copy non-writeback, generated-name safety, and stable negative
diagnostics. The full frontend, ACIR lit, and native CTest suites pass. A
representative indexed shorthand case also reaches frozen QueueGraph, emits the
gfsim C++ bundle, and compiles both generated translation units as C++20.

See `commands.txt`, the adjacent bounded logs, and `summary.json` for exact
evidence.
