# M4 private design-file bridge — verification evidence (2026-09-29)

Scope: modernization-plan revision 8, bounded M4 packet
[`docs/work-items/m4-design-file-bridge.md`](../../../work-items/m4-design-file-bridge.md).
This verifies the private file bridge and the shared stage-aware logical-type fix.
It is **not** public CLI/SDK delivery, **not** an independent design/testbench
delivery, and **not** M5 cutover.

Candidate identity and file hashes are in `candidate.json`. Base is the pushed
`6b514f90` on `codex/gfsim-source-units` plus the six-file uncommitted overlay.

## What is actually claimed

- Explicit, separately compiled source body/interface pairs are read from disk,
  linked and materialized into one verified final design file (`--target final`).
- That file is reparsed in a **separate process** and consumed by the same final
  verifier and the real C++ and Verilog emitters (`--target cpp|verilog`).
- No fixture name is hard-coded, no hidden source body is read, there is no
  QueueGraph/PYC fallback, and there is no whole-system compile followed by
  splitting.
- Any pre-existing `--output` (file, directory, symlink, dangling symlink) is
  rejected without mutation, and validation precedes output creation.

## Shared-compiler fix and why it is needed

`FinalProgram.cpp` (final materialization) strips `ac.logical_element`/`ac.shape`
and sets `ac.logical_type` on the final regs. `ProposalGraph.cpp` previously read
`ac.logical_element` unconditionally, so the reparse path for an already-final
design could not resolve logical types. The fix selects the attribute by the
enclosing module's `ac.stage` (`source` → `ac.logical_element`,
`final` → `ac.logical_type`) and **fails closed** when `ac.stage` is absent or is
neither of those two values.

This is a compiler-private analysis fix inside the bounded M4 bridge scope. It
adds no `ac.*` op, type, attribute, CLI flag, runtime or schema surface: the
ODS diff for this overlay is empty.

## Verification

Current-checkout LLVM/MLIR 22.1.8, macOS arm64. Both emitters are actually
compiled and executed (C++ compiler and Icarus Verilog), not text-only checks.

| Lane | Result |
| --- | --- |
| 10 native C++ binaries (bounded-M2 lane) | **132 tests, 0 failures / 0 errors / 0 skipped / 0 disabled** (`native-summary.json`, per-binary `.xml`/`.log`) |
| All 20 `ACIR*Tests` binaries (widened sweep) | **355 tests, 0 failures / 0 errors / 0 skipped / 0 disabled** (`all-native/all-native-summary.json`) |
| Python system selectors | **52 passed, 0 failed, 0 skipped**; 2 V44 cases deliberately deselected and still DEFERRED to M6 (`python.xml`) |
| Style/whitespace | `git diff --check` clean; `clang-format --dry-run --Werror` clean on both changed/new C++ files |

The widened sweep exists because `ProposalGraph.cpp` is shared compiler code, so the
bounded-M2 ten-binary lane is not sufficient evidence. `ACIRBackendClosureTests`
requires `ACIR_BACKEND_CLOSURE_HARNESS` and `ACIR_SOURCE_UNIT_HARNESS` in the
environment; without them its W10 strict selectors fail by design with
"W10 RED: set ACIR_BACKEND_CLOSURE_HARNESS ..." and that is not a product
regression. With the harnesses set it is 6/6.

A source-only `ac.stage` audit accompanies the shared change: production code
assigns only `"source"` (`PythonImportRecords.cpp:82`) and `"final"`
(`FinalHardware.cpp:388`, `FinalProgram.cpp:1206`). A synthetic `"linked"` value
appears only inside `SourceMathContractsTest.cpp`, which does not route through
`buildSourceProposalGraph`. The new fail-closed stage check therefore rejects no
production stage that existed before, and all 355 native cases confirm it.

The 52 Python cases are the 25 new `tests/system/test_source_design_bridge.py`
cases plus the 27 bounded-M2 selectors
(`test_v41_v42_source_fixtures.py`, `test_source_numeric_next.py`,
`test_unified_register_backends.py -k 'not v44'`). Re-running the M2 selectors on
this candidate is the required check for the shared `ProposalGraph.cpp` change;
they are unchanged and green.

Adversarial coverage carried by the new test file includes: duplicate and
unequal body/header pairs, a top outside the captured source closure, an illegal
final rejected **before** existing bytes change, rejection of every pre-existing
output kind in all three modes, mixed/duplicate/unknown options, and a final
design whose `ac.logical_type` is replaced by source-stage `ac.logical_element`
(which must fail in both backends).

## Independent review

Reviewer: a separate reviewer agent in an independent context, not the author of
any file in this overlay. Verdict on candidate `ede5aec7`: **FAIL**. The
production bridge and the stage-aware fix were judged minimal and fail-closed,
with no output-corruption, invalid-final-admission or new-IR-surface defect; FAIL
was driven by two negative tests that passed without proving what they claimed.
All findings were fixed and re-verified on this candidate.

| # | Finding | Severity | Resolution |
| --- | --- | --- | --- |
| 1 | `test_emit_rejects_illegal_final_before_changing_existing_bytes` was fully vacuous — the output already existed, so `publishNoClobber` refused with `File exists` whether or not the illegal final was rejected | blocker | renamed to `..._before_creating_output`; emits to a non-existent path, asserts `final hardware package envelope is not canonical` and `not output.exists()` |
| 2 | `test_final_logical_type_cannot_be_replaced_by_source_metadata` asserted `"logical" in stderr`, which only matched the tmp_path echoed in `cannot parse final design '<path>'` | major | asserts the real diagnostic `'ac.reg' op final register metadata is incomplete or mixed` |
| 3 | `!logical` returned a bare `failure()` with no diagnostic | minor | emits `proposal reg has no stage-matching logical type '<attribute>'` |
| 4 | The `ac.stage` check sat inside the reg loop, so a reg-less module was never validated, contrary to the stated contract | minor | stage is resolved and validated once per instance view, before the reg loop |
| 5 | `getParentOfType<ModuleOp>()` was recomputed for every reg | minor | hoisted out of the loop |
| 6 | `test_link_rejects_duplicate_source_or_unequal_unit_pair_before_output` asserted only a non-empty stderr | minor (weak check) | asserts the specific diagnostic for each case |

The reviewer also established — and this was confirmed independently — that the
new stage-mismatch branches are **defence in depth** and unreachable through the
design harness: `verifyFinalReg` (`ACIRFinalContracts.cpp:310-321`) and the final
package envelope reject a final reg that lacks `ac.logical_type` or still carries
`ac.logical_element`, and the source path is pre-checked by
`preflightSourceLinkPair` (`SourceLink.cpp:22-39`). Observed on this candidate:
both mutations fail with `'ac.reg' op final register metadata is incomplete or
mixed`, and a non-final `ac.stage` fails with
`final hardware package envelope is not canonical`. The branches are kept
because they make the analysis fail closed on its own contract rather than
relying on a caller's earlier check.

**Negative control — the fix is load-bearing.** Reverting `ProposalGraph.cpp` to
`6b514f90` and rebuilding makes the positive test
`test_m2_source_design_file_is_one_input_to_both_emitters` fail with
`error: proposal StateID has inconsistent exact LogicalTypes`. The stage-aware
selection is therefore required to reparse and emit a final design, and is not
an unexercised extra.

Verification evidence above is PM-run on the frozen candidate and does not
replace this review.

## Artifact naming correction

Per the user's direction, the canonical hardware-design artifact name is
`design_top.ac`, and the private tool was renamed to `acir-design-harness` with
`--design` (no legacy alias retained). The bounded positive fixture is a
closed self-testing system, so it writes `closed_system_testbench.ac`; it is
**not** renamed to `design_top.ac`, because renaming a stimulus/check hierarchy
would not make it an independent DUT. The internal `FinalProgram` C++ container
name is unchanged and no `ac.program`/`ac.design`/`ac.testbench` primitive was
introduced.

## Reproduction

```bash
P=/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit
cd "$P"
M2_BUILD="$P/.pycircuit_out/w10-pm/build"
GATE="$P/docs/gates/logs/20260929-m4-design-bridge"
M2_TARGETS="ACIRRegContractsTests ACIRModuleGraphTests ACIRProposalContractsTests ACIRRegRuntimeTests ACIRSystemLifecycleTests ACIRSimExecutorTests ACIRCheckContractsTests ACIRObservationContractsTests ACIRFinalProgramTests ACIRExecutableBackendClosureTests"
cmake --build "$M2_BUILD" --target acir-source-unit-harness acir-design-harness $M2_TARGETS -j 4
for t in $M2_TARGETS; do
  "$M2_BUILD/bin/$t" --gtest_output="xml:$GATE/$t.xml" > "$GATE/$t.log" 2>&1 || exit 1
done
ACIR_SOURCE_UNIT_HARNESS="$M2_BUILD/bin/acir-source-unit-harness" \
ACIR_BACKEND_CLOSURE_HARNESS="$M2_BUILD/bin/acir-backend-closure-harness" \
ACIR_DESIGN_HARNESS="$M2_BUILD/bin/acir-design-harness" \
PYTHONPATH="$P/python/pycircuit/src:$P/python/semantic-core/src:$P/python/agentic-circuit/src" \
pytest -q tests/system/test_source_design_bridge.py \
  tests/system/test_v41_v42_source_fixtures.py \
  tests/system/test_source_numeric_next.py \
  tests/system/test_unified_register_backends.py -k 'not v44' \
  --junitxml="$GATE/python.xml"
```

Use this checkout's already configured LLVM/MLIR build. Do not copy binaries or
shared libraries from another worktree. Inspect raw XML for skips/disabled cases
as well as process exit status.

## Non-claims and remaining work

- M4 independent design/testbench delivery is **not** established. The audit
  `docs/reviews/20260929-design-testbench-ir-authority.md` (planning checkout)
  records that the selected root still contains stimulus/phase/check/report
  rules, so the positive fixture is closed-system evidence. Proving the design /
  testbench / framework boundary needs separately produced artifacts and would
  require review and, for any new role/op/port/runtime protocol, prior user
  approval.
- `ac.expect`'s complete ODS field schema still lacks a per-field approval
  mapping; no schema extension is made or authorized here.
- The generated-Verilog file-role split between the hardware `FinalModel` and the
  simulation observation wrapper is still open.
- Public driver, per-source generated C++ groups/CMake, standard runner/C ABI,
  managed publication receipts, installation and parallel simulation remain
  later M4/M5/M6 work. This bridge does not replace them.
