# M3-E01 design, oracle and precise approval

Date: 2026-10-01. Base `9ff015a2`. Proposal B SHA-256
`ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`
was independently approval-ready and explicitly approved by the user.
The exact approval is `docs/rfcs/migration/approvals/c2-decl-record-final.md`.

`design-manifest.json` binds the proposal and final oracle MD/JSON.
`revision-a-review.txt` retains rejected return-path/selector gaps;
`revision-b-review.txt` records their exact closure. `oracle-alignment.txt`
closes the supporting scope correction after the B review. `oracle.txt/json`
use the current source-unit driver and preserve the literal
`(3,17)→(17,4)→(4,18)` current/proposal/transfer/reset semantics and adversarial cases.

No compiler/backend record feature was implemented or run in this design step.
Document syntax/JSON parsing/hashes and independent design checks are not a
capability PASS. A new IR attribute and explicitly widened existing shapes are
approved in the proposal, with no new op/CLI/runtime ABI. Existing record/R1
semantics are reused; unsupported ports/static/multi-driver/SYSTEM/EXPECT B
remain outside this precise approval. Plan low-agent implementation only against
these frozen bytes, retaining independent tests and candidate-bound review.

Constructor provenance spelling in the approved raw Markdown triggers MD050;
its exact path is exempt from that style hook to preserve user-approved bytes.
All other artifact/profile/semantic verification remains required.
