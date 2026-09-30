# Language and API reference

The only active source reference is the bounded function-module profile in the
[language specification](language.md): portless `@module`, nested `@rule`,
explicit registration, one default clock, finite scalar state, and empty static
arguments. The public driver is `pycircuit compile`, `pycircuit link`, and
`pycircuit emit`.

The older CycleAwareSignal, structural builder, testbench, PYC, sidecar, and
Agentic Circuit reference pages are retained as short retirement records so
historical links remain understandable. They contain no active recipes.

## Current pages

- [Language specification](language.md)
- [Diagnostics and rejection boundary](diagnostics.md)
- [Source and generated naming](name-mangling.md)

## Retired surfaces and backlog

- Frontend builders, primitives, typed specs, collections, and `@const`:
  retired API pages pending any separately approved replacement.
- Testbench and sidecar schedule: retired; complete `@system`/EXPECT contract
  remains unapproved.
- PYC IR: retired as a public compiler route; the compiler's internal common
  hardware representation is not a source-level user API.

See the [M5 migration guide](../development/m5-migration.md) for unsupported
capabilities and migration scope. Historical RFCS and ACIR records retain their
own historical context and are not active support claims.
