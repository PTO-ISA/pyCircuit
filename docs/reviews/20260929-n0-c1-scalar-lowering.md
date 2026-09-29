# Numeric N0-C1 scalar lowering review

Date: 2026-09-29. Candidate checkout:
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`, HEAD
`82f161ea95336fdd15c238e103f13b9f823e8f28` plus the preserved migration
overlay.

## Scope and candidate binding

N0-C1 adds the private `ac-lower-exact-input-add` pass and
`lowerExactInputAddTransactional(ModuleOp)` helper for the verified source
profile `SourceRead -> from_bits`, constant, and add. It does not register the
pass in the public CLI or admit numeric carriers to ProposalGraph,
FinalProgram, C++, or Verilog.

The final reviewed content is bound to:

- `ScalarNumericLowering.h`:
  `61ea9fe94cb25b912f4ebbadb0207f117db70df13cfb8429e85246a5d6b772cb`
- `ScalarNumericLowering.cpp`:
  `7dda13148f2515d5ae30c1f1fcc53baef1f06da7c4b3c9872ef8c770b76e5e1f`
- `ScalarNumericLoweringAnalysis.cpp`:
  `c300466377fc093b1996616332869310703a43583826ac71323d07483322d742`
- `ScalarNumericLoweringDetail.h`:
  `edb4ab3fcf49387ec924f1a2c4c80a39b4211de4c41c20d3b4ac8815bac24552`
- `LowerExactInputAdd.cpp`:
  `e344975b08b3a7063066cbf5566fa0ce48f106cb39c71f7b292538974b380166`
- `ACIRNumericProofOps.cpp`:
  `84144249096fbf33e15d0223e188a377d8875248da5f24b71f73fb31de13eeed`
- `ScalarNumericLoweringTest.cpp`:
  `907b2a27c15a5a4d6e9564f487782b4575e9507552bea1f62bea955ce6411843`

Implementation used a bounded Sol/medium instance. Independent tests used a
separate Sol/medium instance. Final code review used Sol/high, and architecture
conformance used an independent Astra/xhigh instance. The PM performed the
integration build and final gates.

## Accepted behavior

- Only source implementation units enter the lowering.
- The existing canonical `ac.proof_scope` is required; the pass does not
  synthesize one.
- I is derived from the authoritative SourceRead origin at slot zero. Supplied
  F, C, and S ValueIDs, including nonzero slots, are preserved.
- The pass clones the whole unit, lowers every supported rule, verifies the
  complete clone and its independent B1b witnesses, then replaces only the
  root attributes and body. Failure keeps caller IR byte-equivalent, and the
  root `ModuleOp` identity is preserved.
- Exact arbitrary-precision interval arithmetic selects canonical signedness
  and minimum i1..i64 storage. Each operand receives its own optional
  `extui`/`extsi`; the final `arith.addi` has no overflow flags.
- The result contains SourceRead, the finite constant and arithmetic SSA, four
  I/F/C/S bindings, and one B1b proof per lowered rule. Source math carriers
  and dead control constants are removed.
- No-op and valid C1-lowered units are idempotent. Damaged output, unsupported
  inventory, and source math mixed with any existing ValueBinding or
  NumericProof reject.

Architecture review initially found a repeated-control lifetime defect and an
under-classified mixed-evidence boundary. The final candidate deduplicates raw
control operation pointers before deletion and treats every binding or proof
as evidence for mixed-source rejection. Independent regressions cover shared
lhs/rhs validity, three distinct true controls, B0 evidence in a second rule,
two valid source rules, a later-rule capability failure, and root identity.

## Evidence and verdict

```text
cmake --build .pycircuit_out/w10-pm/build --target \
  ACIRSourceContractsTests ACIRSourceMathContractsTests ACIRSourceUnitTests \
  ACIRNamespaceContractsTests ACIRSourceModuleContractsTests \
  ACIRSourceLinkAdmissionTests ACIRRegContractsTests ACIRSourceFactsTests \
  ACIRRuleEffectsTests ACIRModuleGraphTests ACIRFinalProgramTests \
  ACIRProposalContractsTests ACIRRegRuntimeTests ACIRSystemLifecycleTests \
  ACIRObservationTests ACIRObservationContractsTests ACIRCheckContractsTests \
  ACIRBackendClosureTests ACIRExecutableBackendClosureTests -j4

ctest --test-dir .pycircuit_out/w10-pm/build --output-on-failure
```

Results:

- scalar lowering regressions: 11/11 passed;
- `ACIRSourceMathContractsTests`: 45/45 passed;
- complete configured ACIR lane: 19/19 CTest targets passed;
- clang-format dry run and `git diff --check`: passed;
- all new handwritten native files remain below 600 lines.

The repository-wide default build was also attempted. It remains blocked in a
separate PYC TableGen lane because `PYCOps.td` references the undefined
`ACIR_SourceOwnerAttr`. The N0-C1 and all configured ACIR targets build and
pass; this unrelated PYC integration gap is not counted as N0-C1 evidence.

Independent architecture verdict: **CLEAR / PASS**. Independent code-review
verdict: **APPROVE / PASS**. N0-C1 is accepted. W10 remains open for remaining
operators, Python production, range/check lowering, graph/final/backend numeric
admission, and V41-V44. W11 remains blocked.
