# Source-unit intrinsic pair validation

Status: verified; independent review APPROVE. Base `1680eef746f13deef7a12c32ac26d6c6c72ce365` in
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
The user explicitly requested implementation with subagents after the review
found that source-unit verification only verified the two files independently.
This repairs existing C3-C §143/§181 admission; no IR/public API/schema change.

## Goal and preserved boundaries

Reject wrong source envelopes, interface-as-body, same-owner mismatched owning
headers and retained body/header snapshot disagreement before owner reporting,
replacement or recovery. Reuse native source-body/header contract checks.

A prior unit with internally consistent snapshots remains valid even when its
external providers have changed. Do not fetch current provider headers, promote
snapshots into authority or weaken full source/link authority checks. Stable
header-only compile, declarations-only units (including legitimate empty bodies),
and A/C program verification/recovery behavior remain unchanged.

## Ownership

- `unit_pair_fix`: gpt-6-luna/high. Own SourceBodySnapshots.cpp, SourceUnit.h and
  the native design harness verify-only integration; additional files require
  PM assignment. No shared builds/commits or recursive delegation.
- `unit_pair_tests`: separate gpt-6-luna/high. Own new
  tests/system/test_source_unit_pair_verification.py. Run baseline negatives
  against the unchanged built helper before integration rebuild.
- PM: CMake if needed, shared checkout-local build, integration, evidence/docs.
- Independent architect: read-only design advice; code reviewer reviews the
  frozen result separately from implementation/test authors.

Source freeze precedes final build. Preserve other agents' edits. One writer
per file, shared build `.pycircuit_out/w10-pm/build` owned by PM.

## Acceptance

Real Python/native fixture producers; baseline failures for empty/wrong body
and same-owner cross-version pairing. Native verify-only must reject without
reporting an owner, and public compile --replace must preserve artifact/control
contents. Positive tests cover implementation and declaration units, stale but
self-consistent provider snapshots, normal replacement and header-only reading.
Scope targets: acir-design-harness, source unit/body snapshot/link admission,
artifact verification/recovery, source compile/reader and driver regressions.
No full release, Windows or broad fault matrix claim. Record fresh XML/logs,
commands, candidate hashes and independent review before accepting the fix.

PM explicitly expanded ownership to SourceNamespace.cpp/.h for extracting the
existing registry-independent record shape/site/order validation; full registry
authority checks remain. A subsequent public regression required preserving the
existing owner-mismatch diagnostic via a private structured helper status and
_native_verify.py mapping; no stderr-text classification or public exit/schema
change is introduced. An independent review also found the owning-definition to
import-snapshot mutation; the final candidate rejects it and adds separate private
and public regression cases plus a genuine facade reexport positive.

Final evidence: docs/gates/logs/20260930-source-unit-pair/. Public/unit regression
243 passed with 3 existing Windows skips; native 355/355; strengthened 19-case
pair suite 19/19. Final hashes cover eight code files and the independent test.
Additional PM-assigned extraction files were SourceHeaderHelpers.h and
SourceHeaderRegistry.cpp, to reuse qualified declaration identity rather than
maintain a second algorithm. No other agent's planning edits were touched.
