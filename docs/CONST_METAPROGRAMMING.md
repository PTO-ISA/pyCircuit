# Compile-time metaprogramming (`@const`)

`@const` is pyCircuit's explicit compile-time metaprogramming primitive.

## Contract

- `@const` executes during JIT elaboration.
- It must emit zero IR operations.
- It must not mutate module interface/build state.
- Violations raise `JitError` with source-located diagnostics.

## Allowed returns

- `None`, `bool`, `int`, `str`, `LiteralValue`
- containers (`list`, `tuple`, `dict`) of allowed values
- immutable spec objects exposing `__pyc_template_value__()`
- `@spec.valueclass` objects

## Disallowed returns

- `Wire`, `Reg`, `Signal`
- `Connector`, `ConnectorBundle`, `ConnectorStruct`
- mutable/opaque runtime objects without canonical template representation

## Practical patterns

- Build immutable `spec.StructSpec` / module-collection specs in `@const`.
- Derive widths/masks/loop factors in `@const`.
- Keep hardware emission in `@module` / `@function` only.
