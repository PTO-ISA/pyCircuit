# Compile-time metaprogramming status

The former JIT `@const` API is retired. The current driver captures source and
does not execute module functions, decorators, closures, imports, or
compile-time Python callbacks as a model-building route.

Static arguments are empty in the active profile. Use ordinary source values
and annotations supported by the compiler. Unsupported dynamic construction
fails with a diagnostic. See the [language reference](language.md) and
[M5 migration guide](../development/m5-migration.md).
