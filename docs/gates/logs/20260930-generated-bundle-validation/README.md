# Private generated receipt validation — 2026-09-30

Status: accepted; independent code review APPROVE.
Product base: `645b4ff19dbbeeb01eb7842e31e2a4832bdee9e9`.
Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Candidate: the two product/test files in `candidate-sha256.json`, SHA-256
`7c28c677911f2fac58092a8ad41910e8755f2958b5b29d84223872c6505ef8e2`.
Approved contract: [C3-C](../../../rfcs/migration/c3-driver-runtime.md),
[approval](../../../rfcs/migration/approvals/c2-c3-foundation.md).

## Result and boundary

The private validator checks generated.json's exact fields, canonical owner and
entry binding, closed roles, sorted safe paths, unique file/group membership,
and exact on-disk inventory. A managed reader uses existing shared locking and
recovery and returns immutable receipt/file bytes. The current private profile
requires empty static arguments; nonempty arguments reject explicitly.

The artifact validator does not parse generated code, authenticate historical
producers, prove ABI completeness, or recover missing native source inventory.
No manifest producer, public command, wrapper ABI, runtime, native compiler,
transaction engine or compatibility route is added. Full generated publication
must still connect real native inventory, dut.h ABI and CMake/DUT delivery.
Both target strings are validated as file metadata; this is not RTL generation
or dual-backend execution evidence.

## Independent organization

Repository mapping: bundle_next_map, Astra low. Architecture scope validation:
decl_arch_conformance, Astra xhigh, [scope approved](scope-review.md) under existing
C3-C authorization. Implementation: unit_pair_fix, Luna high, only the new Python
module. Independent tests: decl_cpp_impl, separate Luna high; it owns no product
code in this packet. A new test agent was unavailable at the native thread limit.
PM integrated the independent test file, corrected four diagnostic-spelling
expectations (drive/stream and unique/sorted), formatted, executed and archived
the tests. No negative case or hardware/ownership assertion was removed.
Independent review: other_agent_code_review, Sol high, read-only,
[APPROVE](code-review.md); independently replayed all 63 cases.

## Verification

Python 3.14.6 / pytest 9.0.2 on macOS arm64. Commands include checkout and exact
argv in focused-command.json and existing-command.json; both exit 0.

| Lane | Result | Evidence |
| --- | --- | --- |
| Independent generated receipt tests | 63 passed, zero failures/errors/skips | focused-final.xml/log |
| Existing publication/filesystem/source-reader/driver regressions | 155 passed, 3 skipped, zero failures/errors | existing.xml/log |
| Applicable pre-commit | Passed | precommit-final.log |
| Duplicate-key detector mutation | Two intended failures, one UTF-8 case still passes | duplicate-mutant.xml/log |
| Managed-lock bypass mutation | One intended active-lock assertion failure | lock-mutant.xml/log |

The three existing skips require real Windows APIs: rename/replace/directory
flush, reparse-point metadata opening, and directory handle identity. No Windows
validation claim is made. Shared publication/driver/source-unit files are
unchanged; their hashes are in unchanged-shared-files.json. No native rebuild
was required because no native source or active driver path changed.

New cases exercise header-only and ungrouped glue files, valid cpp/verilog
metadata, strict malformed receipt categories, prepared rollback, committed
cleanup-pending reads, next-writer exclusion during cleanup failure, real process
reader/writer exclusion, and every managed snapshot byte read occurring under the
shared lock. Fixtures are synthetic protocol data, not fake executable designs.

Mutations are process-local monkeypatches; source files stay unchanged. Exact
reproducers are archived as text. Bypassing duplicate checking produces two
`DID NOT RAISE` failures on otherwise valid complete receipts. Bypassing the
managed reader's publication lock causes the actual byte-read hook's active-lock
assertion to fail. This is test sensitivity evidence, not a claim that a
pre-existing generated-bundle implementation was repaired.

## Migration position

This closes private receipt/inventory validation and managed publication
integration for the empty-static-argument profile. M4 remains active for native
bundle production, model ABI, generated build/link/run and RTL source ownership.
M2 remains accepted; public emit and hard-break removal stay M5. SYSTEM/EXPECT B
remain outside this authorization. This packet does not change their status.
