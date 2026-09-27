# U02-N1 independent review-a

- Reviewer role/model/effort: `code-reviewer` / `gpt-5.6-sol` / `high`
- Candidate HEAD: `b797eaf77f2bf5d1651e608bc3855d14fac92047`
- Bound source/test files: 17
- Content-manifest SHA-256: `647c8de127dce02510f4b8530efaac9b493f2c347d1d3e6d677525d003f10a2b`
- Independent tests: 116/116 selected, zero skips; CTest 3/3
- Existing coverage: three-category producer/header namespace metadata,
  CPython 3.14.6/UCD16 identifier oracle including 63,488 BMP scalars,
  facade/header-only paths, shadow/import-use retention, and fail-closed basics
- Verdict: REVISE

Required repairs:

1. Make generated-table provenance truthful by enforcing the stated CPython
   3.14.6 producer or recording the actual interpreter / UCD-only source.
2. Add the applicable Unicode/CPython third-party license provenance and notice.
3. Complete N12 nested closed-record negative coverage.
4. Add P10 UTF-8 and numeric-site ordering with an independent expected table.
5. Cover complete cyclic import bindings plus missing/dangling/unsupported
   authority and conflicting provider/name targets for P11/N04/N08/N13.
6. Regenerate to a temporary file and byte-compare it with the committed table.
   This proves reproducibility and drift detection, not independent exhaustive
   correctness for every supplementary Unicode scalar.

The `T()` default rejection is only unsupported-capability evidence; annotation
timing remains an explicit source-contract ambiguity, not an N1 rule or verdict.
Constants, module imports, full link/final cleanup, C3 emit, and backends remain
outside this bounded implementation and review.
