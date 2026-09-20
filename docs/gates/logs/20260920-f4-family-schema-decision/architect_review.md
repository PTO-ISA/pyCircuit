# Architect review: exact finite-family schema

Date: 2026-09-20

Verdict: accept Decision 0277 before family implementation. Decisions 0275 and
0276 freeze the semantic direction but do not define enough concrete syntax or
IR structure to permit independent frontend, ACIR, QueueGraph, PYC, C++, and
RTL work without divergence.

## Frozen schema

- Python signatures are exactly `static_bool()`,
  `static_int(*, width: int, signed: bool)`, `static_enum(EnumType, /)`,
  `static_config(ConfigType, /)`,
  `static_parameter(name, type, /, *, default=_MISSING, constraints=())`,
  `one_of(*values)`, `integer_range(min, max, /)`,
  `case(*bindings: tuple[str, value])`,
  `module_decl(*, source, parameters=(), finite_cases=None)`, and
  `module(*, declaration)`.
- Collections and case bindings are tuple literals. Cases use ordered
  `(name, value)` pairs. Dictionaries, sets, comprehensions, expansions, and
  computed cases are rejected. Child calls use
  `child(runtime_args..., static=case(...))`.
- A declaration is an ellipsis body with runtime ports only; its implementation
  uses `@module(declaration=decl)`. Parameterized families declare cases
  explicitly. A zero-parameter family canonicalizes to one empty case.
- ACIR has dedicated typed AttrDefs for static types/values/configs,
  constraints, declarations/arguments/cases, ownership/interfaces/family
  schema, dependent expressions/arguments, and logical type expressions. No
  dictionary, JSON, or string carrier is authoritative.
- `ac.module` is a symbol/container, not a function-like executable body.
  Ordered non-symbol `ac.module.case` regions own concrete function types,
  block arguments, bodies, and returns. Imports carry the complete schema;
  instances carry complete typed arguments; returns belong directly to cases.
- Case state, resources, rules, proofs, obligations, provenance, and coverage
  use the full `(family, typed arguments, local key)` identity. Local IDs may
  repeat in different cases but cannot prove or discharge each other.
- PYC preserves `pyc.module`, `pyc.module.import`, `pyc.module.case`,
  `pyc.instance`, and `pyc.return`. Each case carries a concrete physical
  function type and an explicit logical-to-physical interface mapping.
  Backends consume that verified carrier rather than concrete function names.
- Migration is a hard break from direct-body modules, dictionaries, suffixed
  concrete symbols, legacy bare module/JIT specialization,
  specialization sidecars, and string `pyc.params` authority.

## Risks controlled

- Exact source syntax prevents host execution and unordered containers from
  silently defining language semantics.
- Dedicated AttrDefs prevent frontend dictionaries or backend strings from
  becoming a second family schema.
- Container/case separation prevents concrete cases from acquiring symbols and
  recreating specialization identity.
- Case-local proof keys prevent facts and obligations from leaking between
  different concrete parameterizations.
- The PYC logical-to-physical mapping preserves nominal identity after
  aggregate/scalar lowering and keeps backends semantic-neutral.

## Required implementation evidence

The implementation must add positive and negative frontend AST tests, AttrDef
parse/print and verifier tests, exact ACIR operation-structure tests, complete
package/QueueGraph/PYC carrier tests, backend family/parity tests, deterministic
emission, and repository absence gates for every removed path. Until that
evidence exists, Decision 0277 remains `gap-in-scope` and family emission stays
blocked.
