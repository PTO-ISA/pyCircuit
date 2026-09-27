# U02-N1 test-c portability verification

Candidate baseline HEAD: `b797eaf77f2bf5d1651e608bc3855d14fac92047`

The recipe-only CPython/UCD checks are now separate from ordinary product/source tests.

## Ordinary pre-3.14 lane

- Ordinary interpreter: CPython 3.12.12, UCD 15.0.0 (`/Users/zhoubot/.local/bin/python3.12`).
- Recipe interpreter: unset.
- Registered CTest targets: 3; no Unicode recipe oracle target.
- Native results: 36/36 source contracts, 14/14 SourceUnit/header tests, 14/14 namespace/fixed-identifier tests = 64/64.
- CTest: 3/3 passed.
- Direct generator guard: CPython 3.12 was rejected with `generator recipe requires CPython 3.14.6; found cpython 3.12.12`.
- Explicitly configuring Python 3.12 as the recipe interpreter failed at CMake configuration with the expected exact-version diagnostic.

## Explicit recipe tooling lane

- Ordinary interpreter remained CPython 3.12.12.
- Explicit recipe interpreter: CPython 3.14.6 with UCD 16.0.0 (`/opt/homebrew/opt/python@3.14/bin/python3.14`).
- Dedicated native oracle: 3/3 passed, including all 63,488 non-surrogate BMP scalar comparisons.
- Generator/provenance pytest: 3/3 passed, zero skips; temporary regeneration was byte-identical.
- Combined CTest graph: 4/4 passed (the three ordinary targets plus the dedicated oracle).

## Static and scope checks

- clang-format, Ruff format/check, and `GIT_WORK_TREE="$PWD" git diff --check` passed.
- Native test files are 509, 325, 170, and 82 lines; each remains below 600 lines.
- Test-b's 126/126 selected behavior/tooling evidence remains preserved under `.pycircuit_out/u02-n1/test-b/`.
- No product, generator, generated-header, or license file was changed for this portability repair.

Scope remains N1 records, type aliases, and value helpers. C3, final link/backend closure, five-category coverage, installed SDK behavior, and full-framework status remain outside this evidence.
