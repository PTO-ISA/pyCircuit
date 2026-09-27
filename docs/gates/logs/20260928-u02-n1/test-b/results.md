# U02-N1 test-b independent verification

Candidate baseline HEAD: `b797eaf77f2bf5d1651e608bc3855d14fac92047`

The review-a gaps were repaired with test-only changes. Fresh current-checkout configuration and build succeeded in `.pycircuit_out/u02-n1/build` using LLVM/MLIR 22.1.8.

- Preserved baseline: 36/36 source contracts, 14/14 SourceUnit/header GTests, and 51/51 packet/source-admission system tests, zero skips (101 tests).
- Native N1 target: 17/17 tests across namespace structures, registry graphs, and Unicode identifiers.
- Harness namespace integration: 5/5 tests.
- Generator/provenance: 3/3 tests, including temporary regeneration byte equality, rejected unapproved interpreter recipe, and adjacent notice content.
- Combined selected behavior and tooling tests: 126/126.
- CTest: 3/3 isolated targets passed.
- Static checks: clang-format, Ruff format/check, and `GIT_WORK_TREE="$PWD" git diff --check` passed.
- File-size limit: native test sources are 509, 325, and 231 lines, each below 600 lines.
- Generated header SHA-256: `2d3b4e123894b22c27c98d2a34ac369b0289b65c6b93593483ecd7b921e1de82`.

New behavioral coverage includes table-driven missing/wrong/unknown nested fields; exact `z`, `é`, `α` UTF-8 ordering with distinct targets; numeric AST index 2-before-10 ordering; header input permutation; complete cyclic import bindings; snapshot authority failure; dangling and record-constructor export failure; and conflicting provider/name target rejection.

No product failure was found. The record-construction default rejection remains capability-only evidence and is not treated as annotation-timing proof.

Scope remains N1 records, type aliases, and value helpers. This evidence does not establish C3, final link/backend closure, five-category coverage, installed SDK behavior, or full-framework green.
