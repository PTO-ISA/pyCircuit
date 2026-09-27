# U02-N1 independent review-c

- Reviewer role/model/effort: `code-reviewer` / `gpt-5.6-sol` / `high`
- Candidate HEAD: `b797eaf77f2bf5d1651e608bc3855d14fac92047`
- Scoped portability-repair files reviewed: 4
- Full candidate files bound: 21
- Content-manifest SHA-256: `0210a487f64b1fe726f80720701292db1a02bb64c97b7f94a0120e43e7819631`
- Production source, generator, generated table, and license: byte-identical to review-b
- Review-a findings 1-6: remain closed
- Review-b portability finding: resolved
- Verdict: PASS

## Scoped conclusion

The ordinary identifier tests no longer execute a CPython 3.14/UCD16 oracle.
`SourceIdentifierTest.cpp` now contains only fixed expected results and malformed
UTF-8 diagnostics, so the ordinary target runs with the configured product/test
interpreter. The CPython-dependent AST, bounded-batch, and exhaustive BMP
comparisons are isolated in `SourceIdentifierOracleTest.cpp` and are registered
only when an explicit recipe interpreter is supplied.

The CMake configuration validates that explicit interpreter as exactly CPython
3.14.6 with UCD 16.0.0 and fails configuration for a mismatched interpreter.
The Python regeneration test launches the generator through the same explicit
recipe interpreter. An unset tooling interpreter produces a visible, narrowly
scoped pytest skip; the acceptance tooling lane supplies it and proves all three
generator/provenance tests with zero skips.

Test-c evidence covers both sides of the boundary:

- Ordinary CPython 3.12.12/UCD15: 64/64 native tests and CTest 3/3, with no
  oracle target.
- Direct generator execution under 3.12 rejects with the exact 3.14.6 recipe
  diagnostic.
- Explicitly selecting Python 3.12 as the recipe interpreter fails CMake
  configuration.
- Ordinary Python remains 3.12.12 while explicit CPython 3.14.6/UCD16 runs the
  dedicated oracle 3/3, generator/provenance 3/3 with zero skips, and CTest 4/4.
- Static/format checks and `git diff --check` pass.
- Integration reran 56/56 source-unit and namespace system tests against the
  newly rebuilt pre-3.14 harness and reproduced the direct Python 3.12
  generator rejection with the full command. Together with 64 ordinary native
  tests, 3 recipe-oracle tests, and 3 generator/provenance tests, the current
  candidate has 126 selected passing checks.

No fallback masks recipe failures: the optional boundary is explicit, invalid
configuration fails visibly, and the acceptance lane exercises both the
ordinary and pinned-recipe paths.

## Scope boundaries

This approval remains limited to N1 record, type-alias, and value-helper
namespace targets. Constants, `ac.module.import`, five-category completion,
final link cleanup, C3 emitters, backends, installed-SDK behavior, and
full-framework status remain outside this candidate and review.
