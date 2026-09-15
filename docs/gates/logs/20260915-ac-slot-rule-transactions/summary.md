# Agentic Circuit slot rule transactions

Date: 2026-09-15

Base: `origin/main` at `982da52c806e77fc3c7d8b89a4f205615f2cda03`

Decision: 0262

## Result

- Module- and system-owned slots can be explicit leading rule resources or
  hidden resources captured by a nested rule.
- Rule reads use the committed `valid` and payload snapshot. Guarded
  `slot.release()` lowers to firing-local `ac.slot.propose_release`.
- QueueGraph and generated GFSim include named Slot activation and transaction
  resources in the same preflight, prepare, publish, cancel, and reset protocol
  as Queue and Table effects.
- Standalone release remains supported. Duplicate rule owners, mixed standalone
  and rule ownership, invalid aliases, scope escapes, and type mismatches fail
  closed.
- Public state examples were reduced to maintained authoring patterns; focused
  semantic examples remain integration fixtures.
- The feature commit rebased cleanly over upstream field-level firing writes
  from #141. The focused field-write and slot-release lowering tests pass
  together.

## Passing evidence

- Current-checkout native build: `acir-opt`, QueueGraph plan/C++/PYC generators,
  `GfsimTests`, and `CodeGenTests` built successfully.
- Native CTest: `GfsimTests`, `ACIROpsTests`, and `CodeGenTests` passed 3/3.
- Focused ACIR lit: slot release lowering plus both upstream firing field-write
  tests passed 3/3.
- Focused frontend slot ownership/lowering tests passed 5/5.
- Slot direct/native E2E passed 2/2, including deterministic QueueGraph and C++
  generation, flat/nested cycle agreement, compilation, and execution.
- The generated diagnostic catalog now registers `ACPY-SLOT-003` and
  `ACPY-SLOT-004`; its read-only check and focused catalog contract test pass.
- The normative ACIR inventory includes `ac.slot.propose_release`; positive and
  negative lit coverage, the generated IR coverage ledger, and its focused
  contract test all pass.

## Checkout-wide gaps

- The complete Python frontend run executed 391 tests: 388 passed, two skipped,
  and one existing typed-state JIT test errored because
  `ac.var.assign_element` reported an unresolved persistent struct schema.
- The complete Agentic lit run discovered 243 tests: 232 passed, two were
  unsupported, and nine existing tests failed in PYC source-provenance,
  hierarchy/function-symbol, memory-endpoint, and process-state verifier paths.
  The three slot/field-write tests selected above all pass.
- QueueGraph integration ran 35 tests: 23 passed, three skipped, and nine
  existing tests failed on unresolved struct schemas, missing explicit writer
  priorities, a stale generated identifier assertion, and temporary source-path
  determinism. The dedicated slot transaction E2E remains green.
- Documentation layout and component checks passed 15/16. The only failure is
  the existing workspace-local `designs/davincioo` consumer directory, which is
  outside this framework change.
- The complete Agentic contract suite passed 48/50. Both remaining failures are
  the same workspace-local `designs/` consumer-directory policy check; the
  diagnostic catalog and IR inventory failures found by CI are closed.
- Strict decision status accepts Decision 0262 and its concrete evidence, then
  stops on the existing missing
  `docs/gates/logs/20260913-structured-multi-input-r1/closure_summary.md`
  reference shared by Decisions 0189 and 0246.

Strict MkDocs, changed-file pre-commit hooks (including Markdown and API
hygiene), and `git diff --check` pass.
