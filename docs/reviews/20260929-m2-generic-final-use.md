# Generic final-use authority review

Candidate: migration-capture checkout, base `82f161ea95336fdd15c238e103f13b9f823e8f28`
plus preserved migration overlay. File content binding is in
`../gates/logs/20260929-m2-composed-source/generic-files.sha256`.

Independent reviewer: native `code-reviewer` instance `n0_c1_review`,
`gpt-5.6-sol`, high. Verdict: **PASS**, after repair.

Original SourceUse identities, values and targets survive finalization as
RequiredUse/ValueBinding/ValueUse/YieldBinding, including multiple uses of one
read. Independent tests destroy compiler state, reparse final hardware in a
fresh context and reject missing, duplicate, redirected, hidden, ill-scoped and
foreign provenance facts. A stale replacement lifetime issue exposed by those
tests was repaired by rebasing graph references before deleting constants.

Review found unknown attributes on retained constants and yield-enable ANDs
were not rejected. The native verifier now closes both attribute dictionaries;
the corresponding two independent mutations reject. Fresh focused tests: 2/2.
The review repair fingerprint reported by the reviewer is
`d8511668c55c3243036b54a2003787a397d4c6ceb5436497606a93cad1ae267e`.

The preceding integrated baseline passed FinalProgram 51/51, generated backend
18/18 and CTest 19/19. Those counts precede ongoing composed-rule implementation
and are not evidence that the new composed lowering or all M2 is complete.

Current complete V41/V42 source programs compile independently with explicit
headers. Four of five focused source tests pass; the fifth successfully compiles
and has an independent test expectation correction pending (three lexical reads,
not two). Numeric final lowering and dynamic five-cycle parity remain open.
