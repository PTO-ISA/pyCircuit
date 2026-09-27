# Independent F02 tests

Test author: baseline_verification, gpt-5.6-sol, medium. Implementation: governance_impl, gpt-5.6-sol, medium. Independent code review: governance_review, gpt-5.6-sol, high.

C2-C approval: docs/rfcs/migration/approvals/c2-c3-foundation.md. F02 adds no new serialized IR/public interface. Architect interface_design (Astra xhigh) defined intrinsic versus nominal-context responsibilities before implementation.

Initial 12 F02 tests covered four dictionary families, exact fields, logical minimal-width boundaries, arbitrary static integers, kind/length/range constraints, nominal identity, cyclic versus shared graphs, ordered record fields, defaults and nested diagnostics. Review required direct Static scalar/list matcher coverage; 5 additional tests prove unbounded/bounded arbitrary-precision integer boundaries, bool/int separation, nested static-list length/kind/range, and present scalar/list defaults. The absent-default path still resolves the expected type.

The test files are 591 and 269 lines. A missing test-dialect load and a temporary-lambda function_ref lifetime warning were corrected in fixtures; no implementation contract failure was found. Final fresh evidence: 28 combined contract GTests, 6 existing type GTests and 4 lit tests pass; formatting/diff checks pass. Review-b is PASS on the seven-file manifest. Actual compiler header/link authority is explicitly outside this injected-resolver test scope.
