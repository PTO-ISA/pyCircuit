# U01 native independent review findings

Verdict: **REVISE**. Reviewer: code-reviewer, gpt-5.6-sol, high.
PM transcribed the independent final report. The 31-file content binding is
recorded in `content-sha256.txt`; base is `6ac45c51`. All findings require fresh
candidate validation after repair. No native acceptance is implied.

| Severity | Frozen file and finding | Required repair |
| --- | --- | --- |
| HIGH | PythonImportRecords.cpp:359 ignores posonly/kwonly/default forms and accepts variadics | Ordered exact binding/default model; reject variadics |
| HIGH | PythonImportRecords.cpp:310 ignores class bases/metaclass/decorators/type parameters and field initializers; helpers also ignore semantic fields | Explicit unsupported-shape rejection before emission |
| HIGH | PythonImportRecords.cpp:440 silently discards undeclared self.extra assignments | Exact declared-field initialization closure |
| HIGH | ACIRPacketOps.cpp:102 casts fields from a later, not-yet-verified record declaration | Checked access and diagnostics, reversed-operation-order test |
| HIGH | SourceHeaderRegistry.cpp:109 does not anchor origin Site.definition; constructor child paths retain class prefix | Definition/field/parameter anchors and declaration-relative constructor paths |
| HIGH | PythonImportRecords.cpp:200 treats all nonzero relative import levels as one | Correct resolution or explicit capability rejection |
| MEDIUM | Source dependencies use import order; snapshots use StringMap iteration | Structural SourceOwner and canonical symbol ordering |
| MEDIUM | PythonImportAST.cpp:11 does not reject reversed same-line byte columns | Check both byte and codepoint ordering |
| MEDIUM | SourceHeaderRegistry.cpp:228 accepts invalid binding sequences/default gaps | Python binding group order and positional default suffix |
| MEDIUM | SourceUnitHeaderTest.cpp:276 lacks direct equivalence/tamper coverage | SSA-alpha and diagnostic-location positive cases; operand/order/body negative cases |
| MEDIUM | PythonImportAST.cpp:197 ignores operations outside capture attribute | Empty transport body and admitted semantic shape validation |
| LOW | PythonImportAST.h relies on transitive standard includes | Direct optional/cstdint/cstddef includes |

The reviewer confirmed that the candidate build excludes old QueueGraph/ACC/
transform/backend targets and has no fallback or install/public-driver claim.
Snapshot comparison uses operation equivalence with location ignoring and role
normalization, and nonempty check templates reject explicitly. These positive
facts do not close the findings above or the unimplemented U03/final/backend
obligations.

Independent test result for this candidate was RED: 36 foundation tests passed,
all seven header tests stopped in fixture setup, and 13 of 23 system tests
passed. The fixture blocker was Modes positional-only collection; the run did
not reach Request/Pair reordered-constructor assertions. Two separate cases
proved false acceptance of variadic constructors.
