# Migration preview

The former M4 preview workflow has been replaced by the active M5 public
compile/link/emit interface. This page remains as a navigation bridge for
existing references; it does not describe a supported legacy route.

Use the [M5 migration guide](m5-migration.md) for the bounded profile, source
unit compilation, link, C++/Verilog emit, runtime installation, and outstanding
migration callers. The old preview materializer and QueueGraph/PYC commands
are not compatibility fallbacks.

The current public source subset is portless function `@module`, nested
`@rule`, one default clock, and empty static arguments. Complete `@system`,
queues, memory/CDC, multiple clocks, four-state source values, and external
ports remain outside the declared profile. No M5 completion claim is made by
this redirect page.
