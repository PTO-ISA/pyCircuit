# C2 revision B independent review

Verdict: revise. Reviewer: architecture_review, independent architect, gpt-6-astra, xhigh. Date: 2026-09-27.

Reviewed file: docs/rfcs/migration/c2-mlir-contract.md.
SHA-256: e3a2237e2c8cbe59a825e911fbab1b7e962b9d152a2152c8dc4b6a8aee9c5c63.
Frozen bytes: revision-b-proposal.txt.

## Remaining findings

- RequiredUse/ValueID and yield_bindings are each matched, but do not bind the next source use to its output target. Swapping two proven values and their yield ValueIDs can preserve all current checks while writing the wrong states. Bind each required next-use to the source target, reference its UseID from the yield mapping, verify output_bindings/resolved StateID, and specify dynamic-list scalarization. Add a same-type, same-enable swapped-output rejection case.
- InitialSpec scalar/repeat/elements cannot encode generic nonuniform list initialization such as `[i for i in range(entries)]` before parameter binding. Add a full-list StaticExpr initializer, evaluate after specialization, verify length and each element type/range/order, and test entries 2/4 plus Reset without whole-project capture.

## Closed findings

Revision B closes the earlier static-frame/helper/StaticExpr schema omissions, concrete SpecKey retention, ProofScope separation, result/valid/path/domain matching, and DFFE data-enable arity. Bank matches the independent C1 oracle. Donor BindQueueNext.cpp lines 349–360 was checked: donor module order is enable,payload, requiring adaptation.

C1-C bytes are unchanged. Both remaining findings refine IR for already accepted source behavior; C2 remains unapproved. C3, hardware extensions and full product validation are outside this verdict.
