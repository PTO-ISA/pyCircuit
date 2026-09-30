# M7-01 bounded local preview review

Date: 2026-10-01. Independent reviewer:
`/root/m5_complete_candidate_review`, gpt-5.6-sol / high.
Implementation/docs author: `m6_incremental_measurement`, gpt-6-luna / high,
with PM integration. Independent test author: `m5_semantic_oracle_migration`,
gpt-6-luna / high. Review did not author the reviewed files.

Verdict: **APPROVE M7-01 for the local macOS 26/arm64 scalar-profile preview**.
Baseline: `25152f9c630de6d7d019edcd6c9ed09ce1bebbfc`.
Fourteen-file aggregate SHA-256:
`8b5e7c857bae3abad72e1d2495e9e21f19dbe0d809672756d0f794368b105250`.
Formula: sorted path + NUL + file SHA-256 + newline; the
[manifest](../gates/logs/20261001-m7-preview/candidate-manifest.json) binds
implementation, gate, test and reproduction-doc bytes. Later status/index docs
are separate PM acceptance records.

## Findings closed

- CMake presets referenced removed options/targets. The corrected four existing
  build flags and four helper/runtime targets are independently checked against
  parsed presets, isolated configuration/cache and actual target resolution.
  The old preset failed first on `pycc`; the final real build executed 112 steps.
- The retirement gate omitted structured presets. It now scans their option
  keys and target references; independent mutations include the production-scan
  call path, so a tested helper cannot remain disconnected from the real gate.
- Current semantic closure omitted accepted M6 gates, and the relocation fixture
  hardcoded an old native build directory. The closure now includes all four M6
  system files and the new preset case; relocation honors the established build
  override and checks current HEAD metadata. M6 prefix is bound by the shared gate
  helper. The exact flow-tool inventory now includes the accepted measurement tool.
- Active docs/comments described retired compiler routes. The runbook now gives
  current build/validation commands, explicit native/install/MLIR tool bindings
  and a disposable packaging environment. Stable workflow permissions, steps,
  schemas, manifest/version policy and publication protections remain unchanged.

Intermediate review caught a test helper's missing timeout argument, two missing
preset flag assertions, stale runbook environment selection and missing packaging
setup. These were fixed before the final binding. The first semantic run had
307 passes and four missing-`mlir-opt` environment failures. A second pytest run
passed 311 cases but a live script edit caused shell EOF and left a stale fail
summary; that run was rejected as acceptance evidence. The final immutable R3
returned zero, passed 311 cases and wrote a fresh pass summary. Both unsuccessful
runs remain archived, with their distinct causes disclosed.

## Final evidence confirmed

- Fresh current-checkout toolchain/install: baseline metadata, Darwin arm64,
  exact LLVM/MLIR 22.1.8; no foreign worktree binaries or old build-cache reuse.
- Native CTest: 20/20. Semantic script: 311 passed, two explicit V44 deselections;
  its JSON `tests: 26` counts fixture files, not pytest cases.
- Unit suite: 324 passed, three real Windows-only skips, 79 marker deselections.
- Independent preset/retirement/layout: 19 passed; public/runtime/model ABI:
  25 passed; moved-prefix: one passed; wheel install smoke: one passed.
- Retirement, repository management, strict decision-status (283 rows, zero
  deferred), all-files hooks and strict MkDocs passed. Decision-status validates
  the registry's structural/evidence requirements, not every future capability.
- Local wheel SHA-256:
  `9707a2dbbf05960ae574f4905a364b86f948a6a654a33b827f3f604909ac35e8`.
  It is explicitly tagged `macosx_26_0_arm64`, with baseline source metadata.

See the [gate index](../gates/logs/20261001-m7-preview/README.md) for raw logs,
commands, identities, limitations and current-candidate bindings.

## Limits

This is local preview acceptance only. Existing `v6.1.0` peels to older
`d4926615bd0e90978a5f8135d320dc702bb0b81a`; no tag, release workflow, release
index, upload or publication was dispatched. Linux/Windows and the formal SDK's
macOS-15 minimum remain unverified by this packet. Source/IR/CLI/schema/ABI and
manifest/version semantics are unchanged; SYSTEM/EXPECT B remains unapproved.

No arbitrary Unicode host-directory guarantee, wider M6 platform/fault/SDK
closure, RSS/throughput, selective backend rebuild or parallel simulation claim.
The two V44 scheduling/reordering tests remain excluded. Overall M6/M7 and stable
release readiness remain distinct from this completed packet.
