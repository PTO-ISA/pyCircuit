# F4 CodeGen Baseline Prerequisite Evidence

This is a prerequisite repair for F4. It does not claim completion of atomic
Queue/state runtime or pointer-owned multi-TU composition, and it does not
change product compiler or runtime semantics.

## Failure classification

A fresh rebuild at `23a12d64ef640fda515303e88738ff52a79d1e06`
reproduced nine failures, each reporting
`verified firing requires exact typed rule footprints`:

1. `RejectsRawMalformedSourceProvenanceBeforeNormalization`
2. `FlatGeneratorPreservesOrderedRepeatedWritesPerOwner`
3. `BorrowsAggregateTableReadsAndMaterializesOutputsBeforeCommit`
4. `OwnerWriteExclusionProofUsesBoundedSharedDagKeys`
5. `RejectsOutOfRangeConstantTableFiringPlan`
6. `RecomputesBoundedFiringIndexConstraints`
7. `RejectsTableFiringPlanTypeAndOwnershipBypasses`
8. `RejectsForgedFrozenFiringBeforePlanExtraction`
9. `PreservesReadableRuleSourceAndLocalNames`

All nine used the same stale `kStatefulFiring` test fixture. The fixture
hand-authored a lowered `ac.firing` and copied the former summary-shaped
attributes, bypassing the canonical live-SSA inference path added by F1. This
is a fixture/harness defect, not a product compiler/runtime bug.

## Repair

- Replaced the direct firing fixture with an equivalent `ac.rule` source
  fixture preserving one input, one always-present output, one static Table
  replacement, the rule condition, stable identity, and source semantics.
- Added a test-only `lowerRulesAndFreezeQueueGraph` helper that runs the
  canonical `addRuleLoweringPipeline` followed by topology freeze.
- Routed only the nine affected tests through that helper. Exact expression
  DAGs, footprints, typed effects, activation, checks, scheduling, and
  provenance are now inferred from live SSA.
- Updated the readable-source test to attach display names and locations to the
  source `ac.rule` before canonical lowering.
- No verifier relaxation, fallback, copied generated summary, opaque/content
  identity, skip, XFAIL, compiler change, or runtime change was introduced.

## Verification

All commands ran from the current checkout on branch
`codex/f4-codegen-baseline`.

```text
cmake --build .pycircuit_out/build-llvm22 --target CodeGenTests CompilerTests GfsimTests ACIRModelAnalysisTests ACIROpsTests acir-opt-internal acc pycc agentic_circuit_native acir-queue-pycgen acir-queue-cxxgen -j4
PASS

ctest --test-dir .pycircuit_out/build-llvm22 --output-on-failure -R '^(CodeGenTests|CompilerTests|GfsimTests|ACIRModelAnalysisTests|ACIROpsTests)$'
PASS: 5/5 suites, including full CodeGenTests

.pycircuit_out/build-llvm22/bin/CodeGenTests --gtest_color=no --gtest_brief=1
PASS: 111/111 (the stacked F3 branch has one more CodeGen test than the stated
110-test prerequisite baseline)

.pycircuit_out/ac-venv/bin/lit -sv .pycircuit_out/build-llvm22/compiler/acir/tests/mlir --filter='(rule-(exact-effect-order|cse-footprints)|rule-effect-graph-(interactions|effect-order)|architecture-obligation(|-inference|-invalid|-closure|-forged|-missing-rule))\\.mlir$'
PASS: 10/10 focused F1/F2/F3 tests

.pycircuit_out/ac-venv/bin/lit -sv .pycircuit_out/build-llvm22/compiler/acir/tests/mlir --filter='(rule-exact-effect-order|rule-cse-footprints|acc-composite-package|acc-composite-fanout-package|rule-table-lowering)\\.mlir$'
PASS: 5/5 focused lowering plus generated bundle/build/link/executable tests

python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md
PASS

git diff --check
PASS
```

The aggregate `check-acir` target was not used because this existing build is
configured to execute `/usr/bin/true` as the lit runner. The focused tests above
were therefore run with the direct venv `lit` executable. The contract checker
was also attempted and reported the known local dependency gap:
`jsonschema is unavailable; install requirements-dev.lock`. Dependencies were
not changed.

One wider exploratory `acc-driver.mlir` run reached all positive C++ bundle and
Verilog steps but exposed pre-existing diagnostic text drift for a missing AC
unit (`AC unit parsing failed` versus the fixture's expected `verified ACIR
parsing failed`). It is outside this fixture-only prerequisite and was not
masked or modified.

## Status

F4 remains `partial`. This evidence closes only the CodeGen baseline
prerequisite and restores full CodeGen closure before F4 product implementation.
The atomic runtime matrix, wide-payload lifetime/sanitizer closure, and
pointer-owned multi-TU cutover remain open.

## Independent review

- Code reviewer: **PASS**. All nine fixtures now use the canonical live rule
  lowering/closure/freeze pipeline; no product verifier or fallback changed.
- Verifier: **PASS**. Baseline 102/111 and repaired 111/111 were reproduced;
  F1-F3 and generated bundle/executable subsets remain green.
