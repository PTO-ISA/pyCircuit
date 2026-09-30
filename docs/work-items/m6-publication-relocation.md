# M6-01: process termination recovery and moved compiler prefix

Status: done for the current macOS-arm64 packet. Accepted: 2026-10-01. Started: 2026-10-01. Baseline: `d351079b` on
`codex/gfsim-source-units`; working tree clean at dispatch.

This bounded M6 round strengthens the already approved C3-C publication and
SDK contracts. It does not add source syntax, IR operations, schema fields,
public fault flags, scheduling behavior or runtime ABI. M5 remains accepted;
this packet does not claim the complete M6/M7 roadmap is done.

## Objectives and limits

- V48: exercise real termination of an owned public-driver process, OS lock
  release, rollback before the commit point, retained new output after commit,
  and repeated recovery interrupted at existing protocol fault points.
- V47: install the current checkout into a fresh prefix, rename that prefix so
  its original path is absent, and exercise clean-environment source-unit
  compilation, link, both emits and Runtime-only executable use.
- Current platform is macOS arm64. The platform matrix and actual parallel
  scheduling remain later M6 work. Existing source and publication semantics
  remain authoritative; no retired compiler supplies a fallback.

## Ownership

| Responsibility | Instance / actual routing | Writable files |
| --- | --- | --- |
| PM/build/integration | current root session | Shared builds, fixes, packet, gate evidence and acceptance |
| Independent crash/recovery tests | m5_semantic_oracle_migration / Luna high | tests/system/test_m6_publication_process_recovery.py; m6-publication fixtures |
| Independent prefix relocation tests | m6_relocated_prefix_tests / Luna high | tests/system/test_m6_relocated_compiler.py; m6-relocation fixtures |
| Strategy/candidate review | m5_complete_candidate_review / Sol high | Read-only review and reproducers |

Test writers preserve other edits and do not recursively delegate. They do
not repair product code to make their assertions pass. Any actual defect goes
to the PM for a bounded implementation fix and independent regression/review.
Native build `.pycircuit_out/m5-root` was configured and built from this checkout;
its toolchain metadata binds `d351079b4e4f2ba254cc57950f27d59c51dd7c38`.
No compiler, runtime library or generated model comes from another worktree.

## Acceptance checklist

- [x] Public compile/link/CPP emit/RTL emit receive real self-SIGKILL at
  preparing, completed-stage, prepared, saved-previous, installed-destination
  and committed milestones; the expected point must actually be reached.
- [x] Persisted journal and destination/stage/previous layout match the
  independently specified C3 state table before any recovery command.
- [x] Public readers/writers recover old artifacts before commit and retain
  new artifacts after commit; exact source-unit/final/bundle bytes are checked.
- [x] At least one recovery can itself be killed and retried successfully;
  old lock inode remains and no partial artifact becomes valid input.
- [x] Reader/writer ordering uses an explicit event/barrier and verifies OS
  lock release rather than inferring blocking from a fixed sleep alone.
- [x] Original installation prefix is absent; no source PYTHONPATH, private
  helper overrides, original-prefix PATH or LLVM/MLIR environment leaks in.
- [x] Both target bundles consume one saved final after source/body/header
  inputs are unavailable; generated consumers request only Runtime with LLVM
  and MLIR discovery disabled and match independent expected observations.
- [x] Dynamic dependency inspection finds relocated bundle/system references
  and no producer/original-prefix dependency on the current platform.
- [x] Focused gates, actual outcomes, environment, byte binding and independent
  review are archived; update milestone progress only after integration.

## Gate commands

After test authors finish and the candidate is frozen:

```sh
export PYCIRCUIT_NATIVE_BUILD="$PWD/.pycircuit_out/m5-root"
export ACIR_SOURCE_UNIT_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-design-harness"
export ACIR_CPP_SOURCE_PARTS_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-cpp-source-parts-harness"
python -m pytest tests/system/test_m6_publication_process_recovery.py tests/system/test_m6_relocated_compiler.py -q
python -m pytest tests/unit/test_publication.py tests/unit/test_publication_fs.py tests/unit/test_driver_commands.py -q
pre-commit run --all-files
mkdocs build --strict
```

If production bytes change, add the narrow gate proving the fix and refresh
review/evidence against that corrective candidate. Test availability alone is
not evidence that recovery or relocation works.

## Acceptance

Independent Sol high review APPROVE and PM gates bind aggregate
`2d65e3c76ad557a3de030a0cdbbc53b4518fa5007c4705a789de4dce44f9dc45`.
[Review](../reviews/20261001-m6-publication-relocation-review.md) and
[evidence](../gates/logs/20261001-m6-process-relocation/README.md) record results:
5 focused system tests passed; regression 131 passed/3 Windows-only skips;
hooks and strict documentation passed. Production bytes remain unchanged.

Bounded V48 first-publication coverage is source-unit-only here; replacement
coverage spans all four artifact routes. Broader new-final/new-bundle failure
matrices and other platforms remain tracked M6 work. M6-01 is complete, not M6
as a whole. The next packet should address a measured incremental-build/scale
baseline or another explicit platform/fault gap within approved contracts.
