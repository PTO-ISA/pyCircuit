# Boolean fields in nominal records

Status: implemented with focused source, authority and dual-target tests;
candidate-wide gates and consumer integration are recorded separately.
Related issue: [#354](https://github.com/PTO-ISA/pyCircuit/issues/354).
This contract alone does not establish candidate-wide acceptance or a completed
consumer migration.

## Source contract

An annotated `flag: bool` field retains the same logical Boolean binding as a
scalar Boolean port. The physical representation remains one bit. Fixed `ac.u1`
and logical `bool` remain distinct even though their packed widths agree.

Explicit constructors, defaults and updates bind Boolean values or expressions
to Boolean fields. Integer `0`/`1` and fixed `u1` actuals do not become Boolean
merely because they fit. Existing contextual aggregate zero and omitted
same-source defaults construct logical `False` leaves. An explicit `flag=0`
still rejects. Imported constructors still require every field explicitly;
this change does not publish provider defaults.

Projection, nested record access, immutable updates, branch joins and ordinary
helper transport retain the declared source kind. Existing fixed-bit field
conversion rules remain unchanged. The extension adds no state, latency,
transaction order, static-parameter support or implicit four-state cast.

## Declaration authority

The existing `ac.struct` field dictionary retains its `name` and physical `type`.
It may additionally contain `source_constraint`, using the existing hardware
constraint schema:

| Property | Required value |
| --- | --- |
| `kind` | `"hardware"` |
| `type` | Equivalent to the field's declared physical type |
| `source_kind` | `"boolean"` |
| `domain` | `#ac.source_domain<{kind = "bool"}>` |

Only this Boolean constraint is admitted on a field in this extension. Its
physical type must resolve to exactly one-bit Bits. Unknown keys, wrong kinds,
wrong domains, different types and other widths reject. A field without this
property preserves the existing interpretation.

The shared nominal-field verifier reuses `verifySourceCallHardwareConstraint`;
both ordinary declarations and transaction branch nominal validation use that
same contract. Importer field bindings reuse `sourceCallPort` and
`applyBindingBoundary`. A new physical Boolean type, a field-name convention,
and a separate conversion or execution route are unnecessary.

The complete dictionary remains declaration authority through published
interfaces, imports, body snapshots, link, transaction lowering and saved
anchors. Changing or dropping the constraint in one declaration copy must fail
the existing exact declaration comparison. Callers cannot supply a replacement
field constraint to reinterpret a provider's nominal type.

Physical layouts, storage and both emitters continue to use the declared field
type. The one-bit representation carries the existing value/known/Z masks; it
does not grant permission to use unknown controls. An unknown reached guard
retains the existing failure and zero-commit behavior.

## Required evidence

Independent tests must cover direct, imported and reexported nested Boolean
records, source-absent consumption, typed helpers, constructor/default/update
bindings, unchanged reconstructed fields and branch joins. Rejection tests must
include explicit Integer/fixed-Bits values at Boolean boundaries, invalid
unused or overridden defaults, malformed field metadata, inconsistent
declaration copies and failed replacement preserving existing outputs.

Generated C++ workers 1/2 and Icarus RTL must execute independent expected
traces from the same verified final artifact. A comparison can demonstrate an
unknown Boolean X; it cannot demonstrate Z transport. Genuine Z evidence uses
a separate host-driven Boolean port/record transport test. State, reset, hold
and discard claims require their own executed cases.

The real consumer provider remains in its owning repository. Its successful
compilation is an additional integration check, not a fixture recognized by the
compiler. A later consumer failure remains a separate gap; this change does not
authorize changing the consumer's logical field types or expected results.
