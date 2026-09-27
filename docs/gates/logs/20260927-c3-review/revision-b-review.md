# C3 revision B independent review

Verdict: revise. Reviewer: architecture_review, independent architect (Oracle), gpt-6-astra, xhigh. Date: 2026-09-27. Read-only; no authorship or implementation.

Proposal: docs/rfcs/migration/c3-driver-runtime.md.
SHA-256: 6d0f2e2e333f7c17b7d11eefb05611fff76c4a65e58288b1e8726e8b2e4292c1.
Frozen bytes: revision-b-proposal.txt.

## Blocking findings

1. Closed publication control entries omit the temporary journal file required by atomic updates. A crash leaves an unrecognized entry, preventing recovery. Initial creation before owner.json publication also lacks recovery rules. Define fixed temp entries/types, make committed journal authoritative, define bootstrap and stable-lock behavior, idempotent recovery interruption, and postcommit persistent cleanup-error returns. Program readers must find/recover control state before checking the possibly absent target; define missing-control inputs. Existing DirectoryPublication.h lines 19–47 does not implement this protocol.
2. RTL parameters sized only to linked values truncate invalid overrides into accepted keys. Example: entries {2,4}, 3-bit parameter, override 10 becomes 2. Preserve exact raw values before acceptance; define signedness/overflow, bool 0/1, X/Z and aggregate leaf validation; reject unknown combinations in supported elaboration/simulation/synthesis rather than depending only on initial fatal. Test accepted value plus 2^N, negative truncation, bool 2 and invalid mixed leaves. Existing VerilogEmitter.cpp lines 1767–1803 emits typed parameters before case checks.

## Confirmed

C ABI status 0..4, StepState 0..3 and 64/16/24-byte layouts match model_api.h. Failed lifecycle, ABI_MISMATCH for wrong size and post-success cycle accounting are identified as changes. Same-owner replacement, non-semantic receipts and one program locking protocol are coherent in direction. SDK scope is clear: retain distribution/platform/runtime ABI; update generator ABI and capabilities consistently with locks/release index. C2-C approved-for-submission bytes remain unchanged, and user approval is still pending.

No interface approval or product validation is implied by this review.
