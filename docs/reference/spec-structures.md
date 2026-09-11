# Spec structures

`pycircuit.spec` provides immutable compile-time structures consumed by `@const` and hardened during JIT elaboration.

## Core types (selected)

- `FieldSpec(name, width, signed=False)`
- `BundleSpec(name, fields)`
- `StructSpec(name, fields)`
- `SigLeafSpec(path, direction, width, signed=False)`
- `SignatureSpec(name, leaves)`
- `ParamSpec`, `ParamSet`, `ParamSpace`
- `DecodeRule`

All spec objects are immutable and canonicalizable via `__pyc_template_value__()`.

## Struct builder and transforms

Builder:

- `spec.struct("name").field("a.b", width=...).field("x", width=...).build()`

Transforms (immutable):

- `add_field(path, ...)`
- `remove_field(path)`
- `rename_field(path, new_name)`
- `select_fields(paths)`
- `drop_fields(paths)`
- `merge(other)`
- `with_prefix(prefix)`
- `with_suffix(suffix)`

## Wiring integration

- `m.inputs(spec, prefix=...)`
- `m.outputs(spec, values, prefix=...)`
- `m.io(signature, prefix=...)`
- `wiring.bind(...)`, `wiring.ports(...)`
- `wiring.unbind(...)`, `wiring.unflatten(...)`
