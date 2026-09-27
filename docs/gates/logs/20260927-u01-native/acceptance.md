# U01 Packet source/header acceptance

Status: accepted for the bounded U01 slice in the isolated migration branch.
Candidate commit: `d6fb408e5808a8d2f3a83667f6be48afa2dbe4d0`.
Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Branch: `codex/gfsim-source-units`.

PM checked every file in independent review C's 34-file manifest against the
working source before staging and committing. The commit contains exactly those
34 reviewed source/build/test files; the candidate checkout was clean afterward.
It is not cherry-picked into the primary product checkout: this schema/build
cutover belongs to the isolated M5 migration candidate until complete closure.

## Independent validation

- Foundation contracts: 36 passed.
- Source-unit/header native tests: 14 passed.
- Real source/Packet system tests: 51 passed, zero skips.
- CTest: 2/2 targets passed; total selected behavior tests: 101.
- Formatting, Ruff and diff checks passed.
- Independent code review C: PASS, zero remaining findings for U01.

Evidence is in `test-e/` and `review-c/`. The review manifest is bound by
`a6c0466007e47c701a6e7fe4314b59a2c9b89ac91ff0d7d485ca9f591a1cc606`.
Previous failed rounds remain separate; their results are not current acceptance.

Implementation: governance_impl (Sol medium) for schema/importer/harness;
u01_header_authority (configured Luna high) for registry/helper validation;
PM integrated CMake registration and the final range-keyword admission guard.
Independent tests: baseline_verification (Sol medium). Independent code review:
governance_review (Sol high). No test author wrote the reviewed implementation.

## Proven behavior

Actual Python Packet source produces reparsable declarations body and owning
interface. A consumer compiles through registered native dialects using explicit
header input with provider Python/body removed. Constructor calls/defaults/
binding modes and field reads preserve nominal identity and declared field order,
including same-type reordered fields. Authority and snapshots are distinguished;
semantic changes reject while diagnostic coordinate changes remain equivalent.
Malformed source transport, invalid signatures, unsupported semantic-bearing
syntax and malformed forward record references fail with diagnostics.

Generated exact AST paths are tested against known source fixtures. The registry
checks structural/contextual origin and provider-path consistency without adding
an exclusive AST-path syntax whitelist or reading child source.

## Explicit remaining work

U01 admits the documented record/declarations subset, not full C1. Receiver
annotations/defaults currently produce capability diagnostics; this is not a
permanent source-language prohibition. General helper computations, state,
modules, rules, instances, source math/check/SSA proofs, link, C3 publication and
installed SDK, C++/Verilog execution and final retirement remain incomplete.
Nonempty check templates explicitly reject. N1 namespace metadata remains
unapproved at this acceptance point and was not implemented.

Old schema-dependent source files/tests remain in-tree but excluded from this
isolated build; exclusion is not M5 deletion evidence. The source-file size
exception for PythonImportRecords.cpp is bounded to U01 and removed when U02-A
extracts shared constructor-signature logic. No publication or installation ran.
