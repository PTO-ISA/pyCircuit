# N1 source/header candidate: review A

Status: REVISE, not accepted. Isolated base b797eaf77 with dirty overlay;
[test manifest](test-a/source-candidate-sha256.txt) binds the 17 changed source,
generator and test files before the pending provenance repair. The SHA-256 is
647c8de127dce02510f4b8530efaac9b493f2c347d1d3e6d677525d003f10a2b.

Independent Sol medium tests: 116 selected assertions/cases passed, zero skips,
CTest 3/3. The Unicode oracle compares 63,488 non-surrogate BMP scalars with
CPython 3.14.6/UCD 16.0.0, plus directed normalization and malformed cases.

Independent Sol high [review A](review-a/summary.md) requires six repairs:
truthful generator provenance, tracked Unicode notice, complete nested-record
rejection matrix, independent UTF-8/numeric ordering oracle, cyclic consumption
and authority cases, and exact table regeneration. Passing tests do not close
missing coverage. Source license repair and independent test B are active.

The local cmake-overwrite investigation (retained in the ignored candidate output directory) was a false alarm: a plain Git diff
followed shared core.worktree to the primary checkout. The physical candidate
CMake remained correct. Candidate Git commands must explicitly set
GIT_WORK_TREE to this worktree; no old-schema test targets were reintroduced.

This slice covers records, aliases and value helpers. Constants/module imports,
real link stale-binding enforcement, final cleanup and both emits remain open.
The unsupported record-construction default test proves capability rejection,
not definition-time name-binding conformance.

## Review B checkpoint

Test B passes 126 selected tests with zero skips and CTest 3/3. Review B closes
all six A findings but requires one test portability repair: exact CPython
3.14.6/UCD16 regeneration/oracle work must use an explicitly configured tooling
interpreter, separate from supported ordinary Python source/runtime tests.
Pre-3.14 ordinary-runtime evidence and exact-recipe evidence are both required.
The candidate remains REVISE; independent test C is active. No source identifier
semantics changed during the provenance/license repair.
