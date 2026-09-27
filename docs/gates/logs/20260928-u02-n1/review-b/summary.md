# U02-N1 independent review-b

- Reviewer role/model/effort: `code-reviewer` / `gpt-5.6-sol` / `high`
- Candidate HEAD: `b797eaf77f2bf5d1651e608bc3855d14fac92047`
- Bound source/test files: 20
- Source-binding manifest SHA-256: `bc886da2933b5a8ee4fce68d3fcfc6656dc3f89dd7a4d5363515c347062b3017`
- Test-b evidence: 126/126 selected tests, zero skips; CTest 3/3; static/format checks passed
- Generated Unicode table SHA-256: `2d3b4e123894b22c27c98d2a34ac369b0289b65c6b93593483ecd7b921e1de82`
- Review-a findings 1-6: closed by inspected repairs and test-b evidence
- Verdict: REVISE

## Finding

### [HIGH] Optional CPython 3.14.6 regeneration checks are coupled to ordinary product test interpreters

Files and lines:

- `tests/python/agentic-circuit/tools/test_source_identifier_unicode_generator.py:28-38`
- `tests/cpp/agentic-circuit/Dialect/ACIR/SourceIdentifierTest.cpp:144-149`
- `tests/cpp/agentic-circuit/Dialect/ACIR/CMakeLists.txt:1-2`
- `tests/cpp/agentic-circuit/Dialect/ACIR/CMakeLists.txt:45-55`
- `pyproject.toml:14-29`

Issue: the Python regeneration test launches the pinned generator through
`sys.executable`, while the native identifier target receives the ordinary
CMake `Python3_EXECUTABLE` and asserts exactly `3.14|16.0.0`. The CMake target
only requests Python 3.11 or newer, and the product metadata supports Python
3.10 through 3.14. Consequently, the normal test suite fails on supported
Python 3.10-3.13 environments even though the exact CPython 3.14.6/UCD16 pin is
documented as an optional table-regeneration recipe that must not constrain
builds or users.

Impact: otherwise-supported product/source configurations cannot run the broad
N1 test suite. The successful test-b run proves behavior only in the selected
3.14 environment and cannot establish the stated cross-version test contract.

Root cause: recipe-only tooling requirements and ordinary product/source test
interpreter selection share the same implicit interpreter path.

Required fix: keep product/source tests runnable with their supported Python
versions, and configure a distinct explicit CPython 3.14.6 recipe interpreter
for regeneration and UCD16 oracle checks, or move those checks into a dedicated
tooling lane. Preserve the exact regeneration byte comparison in that lane.
Run evidence on at least one supported pre-3.14 ordinary interpreter and on the
explicit 3.14.6 recipe interpreter. Exact regeneration establishes
reproducibility and drift detection; it is not independent exhaustive proof for
all supplementary-plane code points.

## Scope boundaries

This verdict does not reopen the six closed review-a findings. The implementation
remains intentionally bounded to record, type-alias, and value-helper namespace
targets. Constants, `ac.module.import`, five-category completion, final link
cleanup, C3 emitters, backends, and installed-SDK behavior remain future work.
The unsupported `T()` default case remains capability evidence only.
