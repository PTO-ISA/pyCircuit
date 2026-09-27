# C2-F01 independent test evidence

Test author: baseline_verification, gpt-5.6-sol, medium. Implementation: governance_impl, gpt-5.6-sol, medium. Candidate content and checkout: candidate.json.

Fresh current-worktree LLVM/MLIR22.1.8 Debug/assertion build; no compiler/library copied from the primary or donor checkout.

Final independent results after fixes:

- ACIRSourceContractsTests: 11/11 pass.
- Source-contract lit (valid and invalid): 2/2 pass.
- ACIRTypesTests: 6/6 pass.
- Existing types-valid/types-invalid lit: 2/2 pass.
- clang-format dry-run/Werror and git diff check: pass.

True RED findings before implementation fixes: real MathInt parser accepted 007, parameterless math_int type accepted parameters, u64 verifier rejected small positive values in wider integer containers, and accepted signless negative values. PM integration probing then found 0 followed by whitespace/newline was rejected as a leading zero; independent tests added valid space/newline/MLIR line-comment roundtrips and preserved 007 rejection.

Test fixture correction: OpBuilder::getIntegerType(width, bool) was accidentally called with a SignednessSemantics enum. The test now uses IntegerType::get(context,width,signedness) and asserts signedness. The earlier purported ui64-maximum failure was a fixture defect and is not attributed to implementation. Block comments were not admitted as valid trivia by the actual MLIR lexer; valid line-comment coverage remains.

Scope limit: these tests prove the F01 attributes/type/private record validators only. Source import, operations/passes, linking, final-stage elimination, code generation, runtime, both-backend behavior and complete migration are not verified by this slice.

Independent review follow-up: Site non-dictionary path elements now emit a diagnostic, verified by a test capturing the actual diagnostic; an unapproved non-empty field-name restriction was removed to match frozen C2 structural records. Direct standard includes were added. Fresh post-fix evidence is 11 new + 6 existing GTests and 4 lit tests, 21 selected tests total.
