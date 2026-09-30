# pyCircuit documentation

pyCircuit captures a bounded Python hardware description, verifies a common
hardware design in MLIR, and emits C++ or Verilog from the saved final artifact.
The active workflow has one route:

```text
Python source units → verified final design → C++ or Verilog
```

The currently approved M5 source profile uses portless function `@module`
definitions, nested `@rule` functions with explicit registration, one default
clock, finite scalar state, and empty static arguments. This is the accepted
contract; candidate verification and hard-break retirement remain in progress
(Decision 0283).

## Start here

- [Install](getting-started/installation.md)
- [Quickstart](getting-started/quickstart.md)
- [Current M5 profile and migration](development/m5-migration.md)
- [Language reference](reference/language.md)
- The source-owned counter example is at `examples/pycircuit/counter/`.

The example builds from an installed prefix and demonstrates per-source
compile, explicit link, both emit targets, and the generated Runtime runner.

## Documentation map

| Section | Purpose |
| --- | --- |
| `getting-started/` | Installation and the current compile/link/emit workflow |
| `reference/` | Current source contract plus explicit retirement notes for older APIs |
| `architecture/` | Current pipeline overview and status of retired architecture features |
| `development/` | Contributor workflow, gates, governance, and migration status |
| `rfcs/` | Decision history and frozen contract proposals |
| `acir/`, `research/` | Historical specifications and research records; not active product routes |
| `gates/` | Decision register and candidate-bound evidence |

## Build profiles

Compiler development requires exact LLVM/MLIR 22.1.8. Generated-model Runtime
consumers use `find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)` and
`pycircuit::pyc6_runtime`; this profile does not require LLVM. See
[installation](getting-started/installation.md).

Unsupported capabilities, including queues, a complete `@system` contract,
memory/CDC, multiple clocks, four-state source values, and external typed
ports, fail closed. They are backlog items requiring their own approved
contracts, not implicit support in the scalar profile.
