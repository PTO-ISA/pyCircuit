# Generated-model runtime

Generated C++ models link the single `pycircuit::pyc6_runtime` target, backed by
`libpyc6_runtime`. A Runtime-only install does not require LLVM or MLIR. The
CompilerDev component is separate and requires exact version 22.1.8.

The generated CMake project builds source-owned implementation translation
units, the `pycircuit_system` runner, and `libpycircuit_dut`. The runner uses a
finite configuration; for example:

```json
{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":3,"schema":"agentic-model-config","version":"1"}
```

Execution is silent unless `--events <path>` or `--events -` is explicitly
selected. The event stream is a tooling output, not a model or Runtime ABI.

The current model profile is portless, single-default-clock scalar state. Full
`@system` EXPECT behavior, external ports, queues, memory/CDC, multiple clocks,
and four-state source values are not claimed. See
[installation](../getting-started/installation.md) and the
[M5 migration guide](../development/m5-migration.md).
