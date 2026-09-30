# M6-02 independent validation review

Date: 2026-10-01. Reviewer: `/root/m5_complete_candidate_review`,
gpt-5.6-sol / high. Implementation: independent gpt-6-luna / high instance
`m6_incremental_measurement`; independent tests: gpt-6-luna / high instance
`m5_semantic_oracle_migration`. PM authored the bounded example/build integration
fixes; the reviewer did not author reviewed implementation or tests.

Verdict: **APPROVE for the bounded M6-02 packet**.
Baseline: `477beae8ee51e6d6d393f2c658ba5d8343eeeff6`.
Ten-file aggregate SHA-256:
`619fe3ae39b5624e7429080f9015b5143acc3f8c9f904b99da3d67c024ae5e4e`.
Formula: sorted path + NUL + file SHA-256 + newline, as recorded in the
[evidence manifest](../gates/logs/20261001-m6-incremental-scale/candidate-manifest.json).
Final raw report SHA-256:
`73a832c38cb1040d40eaae9557701ed16ed6087ff3a136292cc77eca85339dee`.

## Findings and dispositions

- The public counter graph omitted depfile/generated payload byproducts and
  compiler implementation dependencies. PM reproduced clean/rebuild failure;
  exact byproducts and dependencies now close it without deleting publication
  controls. Independent tests verify restored bytes and unchanged lock identities.
- Verilator 5.044 split space-containing source/build paths while serializing
  JSON list elements into CMake text. The reviewer reproduced failure before
  the bounded quoting correction, then independently configured, built and ran
  the corrected bundle with spaces in both directories. The test's earlier
  space-free copy workaround was removed before final acceptance. Generated CMake
  restores the package helper after `verilate()`; no semantic backend was added.
- The first frozen candidate, aggregate
  `30ca0958f5382441c7e7d9dce32ae4f58001c9cc35f7b4b8265d8d505c1250bf`,
  was rejected: 7 tests passed and 2 failed on Python 3.12 because direct helper
  execution let sibling `types.py` shadow the standard library through `pathlib`.
  The implementation actor replaced that import chain with `os.path`/`os.lstat`.
  Lexical `..` is retained so preflight still rejects symlink ancestors; an
  independent no-mutation regression proves this. The counter CMake test now
  pins the helper interpreter to its test interpreter.

The reviewer reran the final ten focused tests under Python 3.12.12 (10 passed),
checked direct Python 3.14 helper execution, Ruff and both diff checks, and
confirmed the aggregate stayed unchanged after testing. All six final measurement
cases were audited, including inventories, no-op executions, invalidation,
deterministic repeated bytes, literal C++ oracles and the distinct-32 RTL oracle.
PM's final focused run passed 10 tests; existing emit/RTL/ABI regression passed
23 tests on unchanged emitter bytes. Independent test authors also ran all ten
under Python 3.12.12 and 3.14.6. See the
[gate index](../gates/logs/20261001-m6-incremental-scale/README.md).

## Acceptance limits

No arbitrary Unicode generated-source/build host-directory support is claimed:
local Verilator 5.044 emits invalid JSON octal escapes for that separate case.
Unicode SourceOwner/name legalization and the earlier relocated Unicode SDK
prefix remain different claims. RSS is not measured. Wall times include
unisolated process/CMake/Ninja/runner startup and establish no performance
threshold. Ninja no-op/invalidation and deterministic bytes are observations,
not a general cache guarantee. Build `-j4` does not prove parallel simulation.

Leaf, interface and toolchain edits still conservatively re-emit bundles and
rebuild all generated C++ targets. This is a measured optimization opportunity.
M6-02 is accepted; overall M6, SYSTEM/EXPECT revision B and M7 remain separate.
