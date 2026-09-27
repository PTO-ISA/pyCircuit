# Independent F03 tests

Implementation: governance_impl, gpt-5.6-sol, medium. Tests: baseline_verification, gpt-5.6-sol, medium. Independent review: governance_review, gpt-5.6-sol, high.

Eight new test groups cover SourceOwner, ExpansionFrame, Occurrence, SpecKey, ValueID, CheckID, ProofScope, OwnerRef, StateID and StateRef. Exact fields, recursive wrong kinds, diagnostics, u32/u64 boundaries, wide containers, Bool rejection, Unit versus ordinal and permitted empty identities are tested. Ordered arrays retain duplicates; no name regex or arbitrary depth/count restriction is added.

Review required host-independent root-path handling. Negative Windows rooted/drive-relative examples and a positive ordinary POSIX colon component now exercise the repair. Final fresh evidence: 36 combined contract GTests, 6 prior type tests, and 4 lit tests passed. Formatting and diff checks passed. This is not header/link/SSA/filesystem-context evidence.
