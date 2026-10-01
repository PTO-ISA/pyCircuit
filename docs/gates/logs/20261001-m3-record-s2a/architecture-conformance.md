# M3-E01 S2A independent architecture conformance

Date: 2026-10-01. Reviewer: `/root/m3_m6_plan_validation`, independent Astra/high architecture-validation instance. The reviewer did not author implementation or tests. This final note supersedes the unbound `inspection.md`.

Verdict: **CONFORMANT for the intermediate finite record rule/op verifier packet only; no blocking architecture findings**.

## Candidate binding

- Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
- Base HEAD: `4c3a4be690f30453acdc551e224e9ef258291ee1`.
- Approved C2-DECL-R B proposal SHA-256: `ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`, independently rehashed unchanged.
- Source manifest: `.pycircuit_out/m3-s2a-source.json`, 15 compiler/ODS/CMake files, aggregate **`95e3469d0cb433ce0b510c263b179d4e860da0227837de22f34fdd68d5032238`**.
- Associated candidate manifest: `.pycircuit_out/m3-s2a-candidate.json`, 19 files including tests, aggregate `eefe3c34b4e0bd3ffccd1f7b5bb58afae5f90fa4988fd861550c02214861d730`.

All file hashes in both manifests were independently recomputed from the current checkout; both aggregates match the declared algorithm `sha256(sorted path + NUL + file sha256hex + LF)`, with zero mismatches. The architecture verdict binds the 15 source files; matching the test manifest is identity evidence, not a test result.

## Conformance findings

1. **Approved schema changes remain contained.** The two existing value-binding/use operands widen to AnyType, with same-candidate strict record closure or explicit signless-integer checks. Existing scalar witness widths are not mistakenly limited to the 64-bit record-field range. `ac.required_records` is the only added semantic attribute; the dialect hook rejects incorrect placement and source/linked use in this slice. No new operation, dialect type, source API, public flag, receipt, runtime/ABI, scheduler or backend-specific semantics appear.
2. **The admitted packet is deliberately narrow.** `ACIRFinalRecordUses.cpp` requires one owned current input and the same owned target, exactly one read/two gets/one create, one next use and one yield contribution. Bindings resolve to the actual current block argument and exact register handle/StateRef/nominal type. The supported create reconstructs the two source fields in declaration order. It is not a verifier for arbitrary record transformations or `(hi,lo+1)` yet. Read/get/create entry paths must be true; next path must be a true/false literal. Numeric, helper/expanded origin/return, check/observe, dynamic guards, scf/index and other operation/evidence shapes remain rejected.
3. **Source and synthetic semantics stay distinct.** The source relation table is sorted, unique and closed; field IDs, ordinals, domains, origins and actual operands must match. `ACIRRecordSelector.cpp` implements approved B's exact n=1 selector: two source-candidate projections, distinct typed zero leaves, two selectors using the same verified AND enable, one synthetic aggregate and exact yield/use inventories. Unauthorized synthetic origins, bindings, observations, cross-target reuse and orphan operations reject. Both fields share one commit enable; no independent field ownership or arbitration is introduced.
4. **Nominal authority stays common.** `resolveFinalRecordDeclaration` searches direct source-owned final units and uses the S1 record-declaration verifier. Both declaration-provider and implementation-provider units are supported; implementation-owned records precede the unit's `ac.module`. A record nested inside an executable module cannot be promoted into authority. Duplicate/ambiguous ownership and competing nominal declarations reject. The source-stage constructor/header lookup path is unchanged.
5. **The final metadata repair avoids an unapproved requirement.** The core no longer requires a fabricated module `ac.domain` field absent from the existing final module shape. Existing control-port/register-domain contracts retain their own validation responsibility. This intermediate rule seam does not certify the surrounding register reset, module or whole hardware package.
6. **Global admission remains closed.** Diff inspection confirms `FinalProgram.cpp`, `FinalEmit.cpp` and `ACIRHardwareClosure.cpp` unchanged from the base. S1 record-state/value hardware rejection, reset/state admission limits and the common CPP/RTL/runner/source-parts emission guard remain in place. Direct rule/op success and source-free rule reparse are explicitly separate from whole-final validity; inspected tests assert whole-final rejection rather than claiming executable record support.

## Verification and acceptance boundary

Performed read-only contract/source/test-strategy inspection, file and aggregate rehashing, unchanged-boundary diff checks, and source `git diff --check` (exit 0). No compiler, native test, backend, build or runtime probe was run by this reviewer. Only disposable review notes under `.pycircuit_out/m3-s2a-conformance/` were written.

Reported independent regression results and the pending S2A native run belong to their own lanes and are not certified by this source review. The three previously established SourceMath/APInt baseline failures remain explicit; this verdict neither waives them nor states that the complete native lane passed.

S2B Python/helper expansion and threaded failure paths, S2C unified record/numeric materialization/freeze, S3 record state/reset/commit, S4 backend execution, full S2/E01 and product-profile expansion remain outside acceptance. PM closure still requires final independent code review and candidate-bound execution evidence. Any change to one of the 15 compiler/ODS/CMake hashes requires impact review and rebinding; test-only changes require their own evidence binding but do not automatically invalidate unchanged source conformance.
