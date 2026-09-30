# M4 generated receipt and managed snapshot validation

Status: done (bounded private validator/managed-reader packet). Product baseline: `645b4ff19dbbeeb01eb7842e31e2a4832bdee9e9`.
Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Approved contract: [C3-C](../rfcs/migration/c3-driver-runtime.md),
[approval](../rfcs/migration/approvals/c2-c3-foundation.md).
Related approved [C2-DECL](../rfcs/migration/approvals/c2-decl-scalar-final.md)
provides header-only source groups. No further interface approval is required.

## Bounded result

Implement private generated.json receipt/inventory validation and a managed
in-memory reader. Reuse existing publication/recovery/locking; integration tests
supply the real generated validator to the existing publisher. This is file
management, not Python hardware semantics or a new transaction engine.

Current private profile admits only empty static arguments, matching current
native link capability. Nonempty arrays fail explicitly; parameterized replacement
is not delivered. Preserve canonical quoted definition strings, exact owner and
target binding, the approved six-field receipt, closed file roles, sorted safe
paths, unique group membership and exact filesystem closure. Header-only groups
are valid. No extension-based file roles or new source-group ordering rule.

The returned snapshot contains immutable receipt/file bytes read under the managed
lock; it does not require subsequent disk reads. Full production inventory must
later be compared with native structured emitter output. A receipt cannot prove
original IR owner completeness, generated-code semantics, ABI completeness, or
historical authorship. No checksums or semantic facts enter generated.json.

## Ownership and independence

- Read-only repository mapping: bundle_next_map, explore, Astra low.
- Architecture scope validation: decl_arch_conformance, Astra xhigh; recommends
  this exact private boundary under C3-C, with no new proposal needed.
- Implementation: unit_pair_fix, Luna high; only new
  python/pycircuit/src/pycircuit/_generated_bundle.py.
- Independent tests: decl_cpp_impl, separate Luna high; only new
  tests/unit/test_generated_bundle*.py. It owns no product implementation in this
  packet. A new test instance could not be created because of the thread limit;
  this existing independent instance was explicitly reassigned.
- PM: docs, integration, focused test execution, candidate binding, acceptance,
  commit/push. No shared publication engine edits are planned.
- Final code review: other_agent_code_review, Sol high, no author changes.

Do not revert others' edits, recursively delegate, or touch native/CLI/runtime/ABI,
SDK, SYSTEM/EXPECT B or public emit. Full C++ bundle publication requires real
dut.h ABI and generated CMake/link integration; RTL source grouping also remains
separate work. Do not publish a placeholder DUT or describe protocol fixtures as
an executable generated design.

## Verification

Independent tests cover malformed/duplicate fields, roles, paths and owners;
missing/extra files and directories; links and special nodes; empty-argument
capability rejection; header-only and ungrouped glue positives; immutable snapshots;
first/same-owner publication, foreign owner refusal, invalid old/new output refusal;
prepared/committed recovery, cleanup-pending reads and reader/writer exclusion.
Synthetic data is a file-protocol oracle only.

Run focused tests plus existing publication, publication filesystem, source-unit
reader and driver unit regression lanes. No native code changes are planned, so
no redundant native rebuild is needed. Archive exact commands, counts, candidate
hashes and review under docs/gates/logs/20260930-generated-bundle-validation/.

## Acceptance

Two-file candidate SHA-256 manifest:
`7c28c677911f2fac58092a8ad41910e8755f2958b5b29d84223872c6505ef8e2`.
[Evidence and independent APPROVE](../gates/logs/20260930-generated-bundle-validation/README.md):
63 new cases passed, 155 existing cases passed with three Windows-only skips.
The reviewer independently repeated all 63. Process-local mutation probes prove
that bypassed duplicate detection and managed locking are caught by the intended
assertions. Product and test hashes remained unchanged during those probes.

PM integration corrected four diagnostic-spelling expectations and applied
Ruff/Black after the independent test author finished the bounded additions;
no negative case or required assertion was removed. Publication engine, native
compiler, active driver, source-unit reader and runtime files remain unchanged.
Full generated production, DUT ABI, generated CMake/build/run and RTL source
ownership remain open; public new emit is still M5.
