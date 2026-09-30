# Testing and gates

Gate claims must match the exact candidate and the active pyCircuit route. The
M5 cutover is still being integrated; the existence of a command or a test file
does not mean the gate has passed. Record raw outcomes and candidate identity
under `docs/gates/logs/<run-id>/` for semantic or release-significant work.

## Contract under validation

The current public workflow is one source per `pycircuit compile`, explicit
unit closure through `pycircuit link`, and `pycircuit emit` to either backend
from the verified final artifact. The bounded supported source profile is
portless function `@module`, nested `@rule`, default clock, finite scalar
state, and empty static arguments. Required boundaries include negative tests
for unsupported sources and no fallback dispatch.

## Focused validation map

| Change | Evidence to collect |
| --- | --- |
| Capture/source semantics | Accepted module/rule fixtures, source provenance, range/type rejection, explicit registration, current/next and child-state identity |
| Unit compile and link | One producer per source, published interface use, complete closure, duplicate/missing/mismatched unit rejection, verified final artifact |
| C++ emit | Source-owned groups and CMake graph, generated `pycircuit_system` and `libpycircuit_dut`, Runtime-only consumer build, finite-run behavior |
| Verilog emit | Same final artifact, source-owned RTL/map inventory, unsupported configuration rejection, target output preservation |
| Runtime/package | Runtime-only install without LLVM discovery; CompilerDev install with LLVM/MLIR 22.1.8; exported target and external consumer smoke |
| Hard break | Source, CLI, CMake, package and installed-payload scans show no active retired frontend/compiler/fallback |
| Documentation | Active docs agree on profile, command path, unsupported contracts and M5 status |

Run the narrowest check covering the changed behavior, then widen to candidate
acceptance requirements. Do not delete an oracle because an implementation is
missing. Historical baseline tests can remain migration evidence, but they do
not define a second product path or automatically block the bounded profile.

## Build profiles

The root build exposes `PYC_BUILD_COMPILER_DEV`, `PYC_BUILD_TESTING`, and
`PYC_BUILD_RUNTIME_LIB`. A Runtime-only package is configured with compiler dev
off, runtime on, and testing off; it must not discover LLVM or MLIR. The
CompilerDev profile requires exactly LLVM/MLIR 22.1.8. Runtime consumers use
`find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)` and
`pycircuit::pyc6_runtime`.

## Reporting

For each gate, state the exact command, exit status, output/evidence path,
candidate revision or content binding, and any skipped or unavailable checks.
Do not report M5 completion from a documentation update, one passing lane, or
the retired semantic closure. See the [M5 migration guide](m5-migration.md)
for the bounded promise and unresolved caller classes.
