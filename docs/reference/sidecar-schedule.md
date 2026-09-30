# Sidecar schedule status

The sidecar testbench schedule API and `pycc --tb-schedule-mode` options are
retired. The current profile does not expose the former testbench schedule
language or promise a full `@system`/EXPECT contract.

Generated runner event capture is a separate optional tool output selected by
`--events`; it is silent by default and does not restore the sidecar ABI. See
[simulation runtime](../architecture/simulation.md) and the
[M5 migration guide](../development/m5-migration.md).
