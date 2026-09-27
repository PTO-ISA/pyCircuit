# C2 revision C independent review

Verdict: approval-ready. No remaining blockers.
Reviewer: architecture_review, independent architect (Oracle), gpt-6-astra, xhigh.
Date: 2026-09-27. Read-only review; no authorship, implementation or subdelegation.

Reviewed path: docs/rfcs/migration/c2-mlir-contract.md.
SHA-256: 387cf52b129f864b87a1b2a388213a3fe36d81c330d94ced0e6696522b58a322.
Frozen proposal: revision-c-proposal.txt.

RequiredUse now fixes the source target; each yield contribution ties UseID to actual SSA, a unique physical output and StateRef/StateID. Mutually exclusive branches and dynamic selections retain complete mappings and reject swapped proven outputs.

InitialSpec.expression carries the entire static list. Specialization checks exact length, each element's type/range/order, with nonuniform initialization and Reset evidence required.

The repairs agree with SpecKey, ProofScope and DFFE/data-enable contracts. Approved C1-C bytes remain unchanged.

This verdict permits requesting user approval of exactly C2-C. It does not approve the interface, C3, hardware extensions or the product. Full migration acceptance remains outstanding.
