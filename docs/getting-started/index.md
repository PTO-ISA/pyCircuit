# Getting Started

Use the active pyCircuit source profile: function `@module` definitions,
nested `@rule` functions, explicit rule registration, one default clock,
finite scalar state, and empty static arguments. The public workflow compiles
each source unit, links the explicit closure, and emits C++ or Verilog from one
verified final design.

## Start here

| Goal | Start here |
| --- | --- |
| Install compiler and Runtime | [Installation](installation.md) |
| Build the checked-in source-owned counter | [Counter tutorial](tutorial.md) |
| Learn the current source contract | [Language reference](../reference/language.md) |
| Understand retired routes and deferred capabilities | [M5 migration guide](../development/m5-migration.md) |
| Choose the Runtime or CompilerDev CMake component | [Installation profiles](installation.md#runtime-only-install) |

## Build profiles

CompilerDev requires exact LLVM/MLIR 22.1.8. A Runtime-only CMake consumer
loads `pycircuit::pyc6_runtime` without LLVM discovery. Generated-model builds
use C++20, CMake, and Ninja. Verilog simulation tooling is an additional
downstream dependency.

## Scope

The current source profile does not promise CycleAwareSignal or structural
builder APIs, a full `@system`/EXPECT contract, queues, memory/CDC, multiple
clocks, four-state source values, external typed ports, or dynamic collections.
Unsupported constructs fail closed; no legacy compiler is selected.
