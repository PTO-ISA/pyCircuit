# Inline helper closure in Table predicate regions

## Scope

This run covers the Decision 0250/0263 contract that a mandatory
`@ac.inline` pure helper used inside a module-owned `table.find(where=...)`
predicate is expanded before the module is handed to native rule lowering.

Nested predicate, key, invariant, table-match, and table-choose expression
emitters now inherit the enclosing module's `inline_pure_helpers` policy. The
fix is frontend-generic and carries no consumer payload or processor behavior.

## Evidence

- `python3 -m unittest tests.python.agentic-circuit.python_frontend.test_queue_frontend -q`
  - 220 tests passed.
- `python3 -m unittest tests.integration.agentic-circuit.e2e.test_pure_helper_parity.PureHelperParityTest.test_inline_helper_in_table_predicate_reaches_native_codegen -v`
  - one focused native test passed;
  - raw ACIR contains no residual `func.call @matches` in the module predicate;
  - QueueGraph C++ generation completed from the same source.
- `python3 tools/agentic-circuit/check-contracts.py`
  - not runnable in the current host Python because `jsonschema` from
    `requirements-dev.lock` is unavailable; no dependency was installed as a
    side effect of this focused fix.
- `python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md`
  - passed.
- `git diff --check`
  - passed.

## Result

The focused Python and native regressions pass. Full release closure remains a
release-workflow responsibility.
