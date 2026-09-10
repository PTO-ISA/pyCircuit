# pyCircuit 6.0.0 release readiness

## Scope

This evidence closes the repository implementation status for Decisions 0232
and 0234 Part A. It does not claim that the 6.0.0 release instance has already
run or that any release bytes have been published.

Decision 0232 has a frozen exact four-wheel map, deterministic two-platform SDK
construction, manifest and release-index schemas, adversarial aggregation
tests, native dependency checks, relocation validation, and installed-consumer
gates. Decision 0234 Part A has a parsed SHA-pinned workflow DAG with one
candidate acceptance barrier, accepted-byte-only publication, annotated-tag
creation after acceptance, and stable-URL post-download verification.

The repository-addressable implementation evidence is in
`docs/gates/logs/20260910-issue61-release-workflow/`. The historical result
correctly remained implemented-unverified while the repository contract was
being completed. Subsequent issue closures and the full AC/PYC/Table gates are
recorded in the 20260910 and 20260911 evidence directories referenced by the
decision status table.

## Status boundary

- Decisions 0232 and 0234 Part A are `implemented-verified`.
- The release workflow must still build and verify the exact candidate on
  Linux x86_64 and macOS arm64 before its acceptance job can succeed.
- Decision 0234 Part B remains a per-release stop condition: the same workflow
  must publish accepted bytes, redownload them from stable GitHub Release URLs,
  verify their hashes, rerun relocated installed-consumer checks on both
  platforms, and retain the immutable final attestation.
- No tag, GitHub Release, GHCR artifact, PyPI wheel, or issue closure is implied
  by this source-level status promotion.

## Rationale

`decision_status_v6.md` tracks implementation status. Requiring the final
release instance to exist before the workflow's initial strict source gate
would create a cycle: candidate construction depends on that source gate, while
the old status text required candidate construction before source verification.
Separating Part A implementation evidence from Part B release-instance evidence
matches Decision 0234 and preserves every publication barrier.
