# Agentic Circuit state example curation

Date: 2026-09-15

## Result

- Reduced the public state catalog to seven Python examples.
- Moved fifteen decision-specific sources to
  `tests/integration/agentic-circuit/e2e/fixtures/state/` without changing their
  system names or behavior.
- Consolidated explicit slot-resource arguments and nested slot capture in one
  public `slot_rule_mailbox.py` example.
- Removed the unreferenced handwritten `table_multi_writer_issue.mlir`; the
  Python fixture and focused MLIR coverage remain authoritative.
- Updated current specifications, Decision prose, status evidence, and test
  source paths. Historical gate logs were not rewritten.

## Passing evidence

- Frontend lowering: all seven public state examples and all sixteen state
  fixtures lowered successfully.
- Slot rule E2E: 2/2 passed, including public native C++ generation/compilation
  and flat/nested runtime equivalence.
- Relocated rule fixture path checks: 2/2 passed for direct-codegen rejection
  and JIT native lowering.
- Relocated rule PYC/Verilog build: 1/1 passed.
- Documentation layout: 8/8 passed.
- Changed-file pre-commit hooks: passed, including Markdown lint and API
  hygiene.
- `git diff --check`: passed.

## Checkout-wide gaps observed

- The Python frontend suite ran 395 tests: 392 passed, 2 skipped, and one
  typed-state JIT test failed because `ac.var.assign_element` reported an
  unresolved persistent struct schema.
- QueueGraph integration ran 35 tests: 23 passed, 3 skipped, and 9 failed in
  the current feature checkout. The failures are semantic/code-generation
  issues (unresolved struct schema, missing writer priority, a stale generated
  identifier assertion, and source-path determinism), not missing curated
  source paths.
- The combined documentation/component run passed 15/16; the remaining policy
  test found the existing out-of-tree consumer directory `designs/davincioo`.
- Strict Decision status accepted the relocated evidence paths, then stopped on
  two pre-existing missing evidence files referenced by Decisions 0189 and
  0246: `docs/gates/logs/20260913-structured-multi-input-r1/closure_summary.md`.
