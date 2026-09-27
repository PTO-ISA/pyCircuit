# N1 three-category source/header acceptance

Accepted isolated candidate: `03625a3dc3b368be9dbac4bfff7ffdb1d3b54466`. PM verified every one of the 21
reviewed files before staging and committing. Independent Sol high review C is
PASS; complete candidate manifest SHA-256:
`0210a487f64b1fe726f80720701292db1a02bb64c97b7f94a0120e43e7819631`.

Implementation: u02_frontend, u01_header_authority and namespace_unicode,
explicit Luna high. Independent tests: baseline_verification, Sol medium.
Independent source/test review: governance_review, Sol high. PM owns integration.

Approved C2-N1-C is unchanged. Actual Packet→Facade→Consumer producers use explicit
export maps and preserve import consumers/providers, original source sites and
canonical declaration ownership. Header-only transitive consumption, metadata
mirrors, stale mappings, closed records, canonical ordering and cyclic header
consumption are independently exercised for records, aliases and value helpers.

## Evidence

Review A's two provenance and four coverage findings were fixed. Review B's
ordinary-Python versus exact Unicode-tooling interpreter issue was fixed.
Original findings and all test rounds remain archived rather than overwritten.

- Ordinary CPython 3.12: 36 foundation + 14 source-unit + 14 namespace/fixed Unicode
  tests passed; CTest 3/3 without the optional Unicode oracle target.
- Explicit CPython 3.14.6/UCD16 tooling: 3 native oracle + 3 generator tests passed;
  the combined native CTest graph passed 4/4. BMP oracle covers 63,488 scalars.
- PM reran 56 system cases with the final rebuilt harness: all passed.
- Combined final selected coverage: 126 passing tests, no required skipped cases.
- Ordinary 3.12 generator call and explicitly misconfigured recipe both reject.
- Regeneration is byte-identical; production tables remain unchanged by test
  portability repair. The table has its adjacent Unicode notice.

The ordinary source interpreter and explicit recipe interpreter are distinct.
A missing tooling interpreter does not invalidate ordinary product tests; the
N1 acceptance gate supplies it explicitly and runs the dedicated tests.

## Remaining scope

Only the source/header subset is accepted. Constants, module imports, real
header/body linker stale-binding checks, namespace removal from final IR,
actual C3 emit paths, both backends, SDK and full framework remain open.
Interleaved annotation/default/body name timing is not newly specified; a
capability rejection does not prove definition-time semantics.

The implementation remains in the isolated migration branch. It has not been
published or substituted for the primary product route; M5 hard-break retirement
is a later whole-candidate obligation.
