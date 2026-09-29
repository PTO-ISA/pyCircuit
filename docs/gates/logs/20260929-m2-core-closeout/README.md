# M2 bounded core acceptance — 2026-09-29

Scope: modernization-plan revision 8 on `codex/gfsim-migration-governance`.
This accepts the bounded M2 core, not the complete migration, public CLI/SDK,
parallel simulation, M5 hard break, or a release. Candidate identity and source
hashes are in `candidate.json`. Base is `ee855c1a` plus the three-file overlay.

## Supported profile and evidence chain

The two closed systems use Python functions and nested rules, bool and bounded
unsigned integers, constant-right add/and_bits/eq/ne/lt, source-owned headers,
module aliases, serial Work/Xfer, reset, checks and the required observations.
ComposedFixture writes actual Python sources and separately captures/compiles
each body against preceding headers. MLIR lowering and source linking create
one verified FinalProgram used by both actual C++ and Verilog executions.
The independent expectations remain `[0,2,5,5,5]` with 4 physical registers and
`[0,0,3,6,6]` with 5; ports introduce no relay register.

## Fixes and independent review

- RTL statistics previously stored a StringRef to a temporary identity string.
  The JSON row now owns its strings; full backend record equality stays intact.
- A registered empty clocked rule is legal under approved C3, Step state table:
  no active write is not QUIESCENT. The obsolete rejection test now checks the
  rule in AnalysisClosed, then instance rule ordinals and HasWork after
  materialization. Existing malformed-graph negatives are unchanged.
- V41/V42 measurement tests now validate the added terminal Result explicitly,
  alongside all previous trace, event, gauge and reset assertions.

Implementation: native `lifecycle_fix`, gpt-6-luna/high. Independent test audit:
`zero_rule_test_audit`, separate gpt-6-luna/high. PM added the independent full
JSON equality and terminal assertions and ran integration checks. Independent
native reviewer `n0_c1_review` returned PASS on the fixes, phase-correct tests
and real source-to-backend evidence chain. This reused review instance's model
was not reselected/reported by the current host API; no new model claim is made.
The final test-only formatting change is nonsemantic and was rebuilt/retested.

## Verification

Current-checkout LLVM/MLIR 22.1.8, macOS arm64. Both emitters are actually
compiled/run (C++ compiler and Icarus Verilog), rather than text-only checks.

- 10 native binaries: 132 tests, zero failures/errors/skips/disabled cases.
- Python/source/backend: 27 passed, zero failures/skips; 2 V44 cases deliberately
  deselected and DEFERRED to M6, never counted as passed.
- clang-format dry run on both changed C++ files and git diff --check: passed.
- Build warnings: existing MLIR deprecated builder calls and duplicate static
  libraries; no build errors.

Raw per-binary XML/logs, Python XML and native-summary.json are adjacent.
`attempt1/` preserves failures from the outdated exact-key assertion and the
incorrect AnalysisClosed instance assertion; trailing spaces in its pytest XML
diagnostic text were normalized for Git whitespace checks. Earlier checkpoint failures remain
in `../20260929-migration-checkpoint/`; they are superseded only for this scope.

## Reproduction from this checkout

Use an already configured checkout-local LLVM/MLIR build at the path below.
Do not copy binaries from another checkout.

```bash
M2_BUILD="$PWD/.pycircuit_out/w10-pm/build"
M2_TARGETS="ACIRRegContractsTests ACIRModuleGraphTests ACIRProposalContractsTests ACIRRegRuntimeTests ACIRSystemLifecycleTests ACIRSimExecutorTests ACIRCheckContractsTests ACIRObservationContractsTests ACIRFinalProgramTests ACIRExecutableBackendClosureTests"
# Run in bash so the explicit target list expands into separate arguments.
cmake --build "$M2_BUILD" --target acir-source-unit-harness acir-backend-closure-harness $M2_TARGETS -j 4
mkdir -p .pycircuit_out/m2-replay
for target in $M2_TARGETS; do
  "$M2_BUILD/bin/$target" --gtest_output="xml:$PWD/.pycircuit_out/m2-replay/$target.xml" || exit 1
done
ACIR_SOURCE_UNIT_HARNESS="$M2_BUILD/bin/acir-source-unit-harness" \
ACIR_BACKEND_CLOSURE_HARNESS="$M2_BUILD/bin/acir-backend-closure-harness" \
PYTHONPATH="$PWD/python/pycircuit/src:$PWD/python/semantic-core/src:$PWD/python/agentic-circuit/src" \
pytest -q tests/system/test_v41_v42_source_fixtures.py tests/system/test_source_numeric_next.py tests/system/test_unified_register_backends.py -k 'not v44' --junitxml=.pycircuit_out/m2-replay/python.xml
```

Inspect raw XML for skips/disabled cases as well as process exit status.

## Remaining work

M4 owns a standard usable compile/link/emit/runner flow and its applicable
per-source C++/TU/CMake delivery. Exposed installation/output paths require
basic smoke and output protection. M6 owns real parallel scheduling and broader
SDK/platform/fault matrices. M3 adds capabilities for a selected use case;
FIFO, memory, CDC, broader numeric/collection and four-state support are not
claimed here. V43's private lifecycle fixture now passes, but that does not
claim complete public C3 runner/ABI delivery. M5 cutover is still pending.
