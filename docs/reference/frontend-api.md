# Frontend API status

This page formerly documented the CycleAwareSignal and structural builder
Python APIs. Those surfaces are retired and are not supported entry points.

The active Python source subset uses function `@module` declarations, nested
`@rule` functions, explicit registration, finite scalar state, and ordinary
Python expressions accepted by the compiler. The public driver is
`pycircuit compile`, `pycircuit link`, and `pycircuit emit`.

Use the [language reference](language.md) for the supported source contract and
the [M5 migration guide](../development/m5-migration.md) for hard-break
boundaries. No compatibility alias is provided.
