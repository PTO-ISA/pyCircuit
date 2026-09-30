# Source-unit intrinsic pair validation — 2026-09-30

Acceptance scope: repair remaining C3-C §143/§181 source-unit replacement and
recovery validation. Base `1680eef746f13deef7a12c32ac26d6c6c72ce365` plus the nine
code/test files in `candidate-files-final.json`. Working directory is
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
No ODS/public CLI/runtime/receipt/journal schema changes; SYSTEM/EXPECT revision
B proposals are not implemented by this patch.

## Change and independence

The private native verifier now validates one source body/interface pair,
including stage/kind/owner, matching exports/import bindings/interfaces, owning
module declaration, root/control prefix, ports/effects and retained snapshots.
It reuses source-link body/header checks. Local declaration identity and namespace
record/category predicates are shared; local retained-target resolution is used
only by intrinsic validation. Full link still uses the actual supplied registry
and provider export authority. No snapshots are promoted to global authority.

Declarations-only bodies may be empty. A declaration belonging to the unit must
be a definition in its owning header; external snapshots keep dependency owners.
Facade import sources may differ from the canonical target owner. Old internally
consistent units do not rebind to current external provider files, and stable
header-only consumption stays unchanged.

A private helper status (3) transports a structurally valid owner mismatch to
Python, preserving the existing public owner diagnostic and CLI exit=1. General
corruption keeps the native-verification diagnostic. Failed validation never
writes a successful owner report; no stderr-string classification is used.

Implementation: `unit_pair_fix`, gpt-6-luna/high. Independent tests:
`unit_pair_tests`, separate gpt-6-luna/high. PM coordinated file ownership,
changed-line formatting, integration, final gate runs and strengthened the new
negative helper to require rc=1 plus an error diagnostic (not any nonzero exit).
Independent reviewer: `other_agent_code_review`, code-reviewer role,
gpt-5.6-sol/high, APPROVE after fixes and confirmation of the test-only tightening;
all nine final hashes independently confirmed. Read-only architecture advice:
`other_agent_arch_review`, architect role, gpt-6-astra/xhigh.

## Evidence and results

- Corrected initial regressions against unchanged baseline native helpers:
  `before.xml/log`, 9 failures and 3 passes. These failures demonstrate actual
  acceptance of incoherent pairs, not invalid fixture syntax.
- Additional paired export/import target mutations on the intermediate candidate:
  `target-before.xml/log`, 4 failures. They change both files together, so simple
  metadata equality cannot satisfy the oracle.
- First integrated public regression: `regressions.xml/log`, 235 passed, one
  failed owner-diagnostic regression, 3 Windows skips. The private structured
  status repair closes that regression without weakening its existing test.
- Final public/unit regression: `regressions-final.xml/log`, 243 passed, 3 skips,
  zero failures/errors. Includes 19 new pair-validation cases.
- Final test-only strengthening: `pair-strict-final.xml`, 19/19 passed. Only the
  rejection assertion changed after the combined run; production files and all
  other selectors are unchanged. Exact rc=1 now excludes signals/usage errors.
- Native: `native-summary.json`, 355 tests across 20 binaries, zero failures,
  errors, skips or disabled cases. First direct BackendClosure run omitted its
  environment variable: its `.missing-env.log/xml` are preserved. Only that
  six-case binary was rerun with the correct harness environment, then the
  aggregate summary was refreshed; this was a runner setup error, not a code fix.
- Build: selected current-checkout ACIR targets, LLVM/MLIR 22.1.8, macOS arm64;
  build exit 0. Existing deprecated-builder and duplicate-library warnings remain.
- Changed-line formatting, Python syntax/lint from the independent test lane,
  and git diff --check passed. Archived log/XML trailing whitespace was normalized;
  diagnostic content and result counts are unchanged.

The 3 skips are only existing Windows filesystem tests:
`test_windows_real_rename_replace_and_directory_flush`,
`test_windows_real_reparse_metadata_open_is_rejected`,
`test_windows_directory_flush_rejects_handle_identity_change`.
They require real Windows APIs, reparse points and directory handles respectively.

## Reproduction

Use Bash and the existing checkout-local configured build. Do not copy tools
from another checkout. `native-targets.json` lists all 20 test targets built.

```bash
PYC_BUILD="$PWD/.pycircuit_out/w10-pm/build"
export ACIR_SOURCE_UNIT_HARNESS="$PYC_BUILD/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PYC_BUILD/bin/acir-design-harness"
export ACIR_BACKEND_CLOSURE_HARNESS="$PYC_BUILD/bin/acir-backend-closure-harness"
export PYTHONPATH="$PWD/python/pycircuit/src:$PWD/python/semantic-core/src:$PWD/python/agentic-circuit/src"
pytest -q tests/system/test_source_unit_pair_verification.py
pytest -q tests/unit/test_artifact_verify.py tests/unit/test_driver_commands.py tests/unit/test_source_compile.py tests/unit/test_source_unit_files.py tests/unit/test_publication.py tests/unit/test_publication_fs.py tests/system/test_artifact_verify_recovery.py tests/system/test_driver_compile_link.py tests/system/test_source_compile_publication.py tests/system/test_source_unit_pair_verification.py
ctest --test-dir "$PYC_BUILD" --output-on-failure -R '^ACIR.*Tests$' -j 4
```

For the original recorded native run, each executable was run directly with
`--gtest_output=xml:<unique-file>` and four independent processes; the environment
above is required for BackendClosureTests. Inspect raw XML, not only exit codes.

## Remaining limits

This closes the reviewed unit-pair admission defects, not all migration work.
No historical-writer authentication, depfile-content validation, Windows lock
validation, public emit/SDK delivery or M5 cutover is claimed. No unapproved
first-class-system/role/EXPECT schema is introduced. Do not mark M4–M7 complete
from this patch or reinterpret the original source-unit header-only contract.
