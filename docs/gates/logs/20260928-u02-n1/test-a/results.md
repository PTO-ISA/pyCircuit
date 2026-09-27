# U02-N1 independent source namespace and identifier verification

Candidate HEAD: `b797eaf77f2bf5d1651e608bc3855d14fac92047`

Fresh current-checkout configuration and build succeeded in `.pycircuit_out/u02-n1/build` using LLVM/MLIR 22.1.8.

- Preserved accepted suite: 36/36 source contracts, 14/14 SourceUnit/header GTests, 51/51 packet/source-admission system tests; zero skips (101 selected baseline tests).
- New independent N1 suite: 10/10 native tests and 5/5 registered-harness system tests (15 new tests).
- Combined selected behavior tests: 116/116.
- CTest: 3/3 isolated targets passed.
- CPython oracle: Python 3.14.6 with Unicode database 16.0.0; every non-surrogate BMP scalar (63,488 values) was compared for single-character identifier acceptance, plus fixed multi-codepoint, supplementary, malformed UTF-8, reserved-name, NFKC, AST parsing, and compile-without-execution cases.
- Formatting/static checks: clang-format, Ruff format/check, and `GIT_WORK_TREE="$PWD" git diff --check` passed.
- Existing manual header fixtures and assertions were unchanged; no product source was edited by the test owner and no commit was created.

The evidence supports only N1 source/header namespace contracts, Unicode identifier validation, and the implemented record/type-alias/value-helper subset. It does not establish C3, final linking/backend closure, five-category coverage, installed SDK behavior, or full-framework green.
