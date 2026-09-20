# Bounded architecture review: F4 static parameter families

## Scope reviewed

This review covers only the static-parameter family decision needed by the F4
pointer-owned composition and F5 readable-family emitters. It does not approve
an implementation, change Decision 0267 equality, or close Decisions 0274 or
0275.

## Accepted architecture

- One source definition owns an ordered `StaticParameterDecl` list and the
  complete explicit `finite_cases` set independently of callers.
- The admitted domain is closed to `bool`, fixed-width signed/unsigned integer,
  nominal enum, and recursively typed nominal immutable configuration values.
- Required/default binding, `one_of`, and `integer_range` are the complete F4
  default/constraint surface.
- The dependent-expression grammar is closed and backend-independent.
- Identity remains `(family symbol, ordered typed static arguments)`; concrete
  cases are non-symbol typed records.
- Source-owned headers, nominal interfaces, and bodies remain authoritative.
- QueueGraph must carry typed declaration/constraint/case records.
- C++ and RTL each emit one readable family. Concrete widths greater than 64
  bits select immutable shared Queue storage per case.
- Open domains, caller-observed inference, dictionaries as declarations,
  suffix/string identity, and dual emission modes are rejected.

## Review verdict

The contract is sufficiently closed for implementation planning and preserves
Decisions 0267, 0270, and 0274 without adding a competing identity. The
acceptance and negative matrices in Decision 0275 are mandatory implementation
evidence. Status remains `gap-in-scope`; no current concrete-symbol WIP is
evidence that the family schema or emitters are complete.

## Deferred boundary

Open/symbolic domains, caller-driven monomorphization, generated ranges,
relational or conditional constraints, collection-valued parameters, and a
richer dependent-expression language require a later decision.
