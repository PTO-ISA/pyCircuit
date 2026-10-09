# Repository map

Start with the owner of the behavior you want to change. The repeated repository-name wrappers are removed. Folder boundaries
separate the compiler, installed Runtime, repository tooling and independent
verification; similar names do not imply duplicate implementations.

## Find the right owner

| Change | Start here |
| --- | --- |
| Python syntax capture, CLI or publication | `python/pycircuit/` |
| Types, hardware semantics or source lowering | `compiler/lib/Compiler/` and `compiler/lib/Dialect/ACIR/` |
| Common IR transformations | `compiler/lib/Transforms/` |
| Generated C++ or Verilog | Existing emitters in `compiler/lib/Compiler/` |
| Runtime lifecycle, scheduling or observations | `include/gfsim/` and `simulator/gfsim/` |
| Standard RTL storage leaves | `include/verilog/` |
| Public build/install integration | Root `CMakeLists.txt`, `cmake/` and `packaging/` |
| Reproduce a compiler or Runtime defect | The owning suite under `tests/` |
| Add or document a complete example | `examples/<design>/` and `examples/catalog.json` |
| Run a gate or measure a build | `flows/scripts/` and `flows/tools/` |
| Regenerate maintained source data | `tools/` |

## Tracked top-level folders

Every tracked source folder belongs to one of these owners. Empty placeholders
and retired compiler paths are absent from the active tree.

| Folder | Responsibility | Navigation |
| --- | --- | --- |
| `.codex/` | Project-local management and independent-review skills; no product code. | [Governance](project-governance.md) |
| `.github/` | CI, issue/PR templates, repository policy and release automation. Actions share setup; workflows retain their distinct trigger and permission boundaries. | [Repository management](repository-management.md) |
| `benchmarks/` | Source-unit workload generation. Measurement drivers remain in `flows/tools/`. | [Benchmark guide](https://github.com/PTO-ISA/pyCircuit/blob/main/benchmarks/README.md) |
| `cmake/` | Installed package configuration and generated-module/example build helpers. | [Installation](../getting-started/installation.md) |
| `compiler/` | Native MLIR compiler, common hardware IR and both emitters. | [Compiler pipeline](../architecture/compiler-pipeline.md) |
| `docs/` | Current guides, language reference, branding and preserved contract history. | [Documentation home](../index.md) |
| `examples/` | Flat, independently buildable hardware designs and their verification assets. | [Example catalog](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/README.md) |
| `flows/` | Repository build, gate, preview and measurement orchestration. | [Testing and gates](testing-and-gates.md) |
| `include/` | Installed C++ Runtime interfaces/templates and standard Verilog storage leaves. | [Runtime semantics](../architecture/simulation.md) |
| `packaging/` | SDK identity, relocation, platform/release validation and wheel assembly. | [Build profiles](../getting-started/installation.md) |
| `python/` | The single importable `pycircuit` package, directly under `python/`. | [Source-unit workflow](source-unit-workflow.md) |
| `schemas/` | Current SDK, consumer-lock, release-index and version-map JSON formats. | [SDK contracts](https://github.com/PTO-ISA/pyCircuit/tree/main/schemas) |
| `simulator/` | Compiled implementation of the shared Runtime library. | [System execution](../architecture/system-execution.md) |
| `tests/` | Compiler, Runtime, public-flow, packaging and unit verification. | [Test ownership below](#test-ownership) |
| `toolchains/` | Pinned LLVM source/version provenance. This is a reference record; build entrypoints perform their own exact version checks. | [LLVM pin](https://github.com/PTO-ISA/pyCircuit/blob/main/toolchains/llvm.lock.json) |
| `tools/` | Source-table generation and example-catalog maintenance. | [Tool guide](https://github.com/PTO-ISA/pyCircuit/blob/main/tools/README.md) |

## Compiler and Runtime boundaries

```text
python/pycircuit/   syntax capture and public driver
compiler/
  include/pycircuit/             public compiler headers and TableGen definitions
  lib/Compiler/                 source resolution, lowering, link and emit
  lib/Dialect/ACIR/              common hardware types, operations and verification
  lib/Transforms/               MLIR transformations
  tools/                        native compiler entrypoints
  cmake/AddACIR.cmake            compiler-private library helper
include/
  gfsim/                        installed Runtime headers and templates
  verilog/                      installed managed storage leaves
simulator/gfsim/                 compiled Runtime implementation
```

`python/pycircuit/` is the importable Python package. Package discovery starts
at `python/`; redundant project and `src` wrapper layers are removed.

`include/gfsim/` and `simulator/gfsim/` jointly implement the Runtime component:
one contains public headers/templates, the other compiled sources. Moving them
into one flat folder would change include and install paths without removing a
responsibility. Likewise, `compiler/include/` serves compiler developers while
root `include/` serves generated-model consumers.

`cmake/` is installed for downstream builds; `compiler/cmake/` is private to the
native compiler build. `flows/tools/` runs repository operations;
`tools/` regenerates maintained source data. These are separate owners,
not alternate public compilation routes.

The retired primitive-selection catalog, its standalone RTL variants and its
private generator/report tests have been removed together. Current count-zero,
popcount and priority operations use the common IR and current source/backend
tests. `include/verilog/` remains the active installed leaf implementation;
there is no second `library/` selection path.

## Test ownership

| Folder | What belongs here |
| --- | --- |
| `tests/unit/` | Python-only and mocked contract tests, including the shared example verifier. |
| `tests/runtime/` | Native Runtime arithmetic, storage, runner and lifecycle tests. |
| `tests/compiler/IR/` | Native tests of common IR, analyses and transformations. |
| `tests/compiler/lit/` | Public compile/link/emit, diagnostics and backend execution scenarios; `Inputs/` contains their source, driver and expected-data fixtures. |
| `tests/compiler/oracles/` | Independent reference models, checkers and immutable historical provenance. |
| `tests/system/` | End-to-end Python tests of installed tools, publication, simulation and packaging. |
| `tests/integration/` | Source trees and consumer projects used by those public-flow/package tests. |

The `history_*` oracle folders and `package-fixture-originals/` snapshots are
intentional verification assets. Their manifests preserve original behavior and
provenance; they are not alternate product implementations. Do not combine an
independent expected model with the DUT merely because the files look similar.

Examples stay flat under `examples/<design>/`. Each folder owns its source,
standalone build, stimulus and oracle. Repeated small `config.json` files support
independent builds and are not shared mutable configuration. `GENERATED.json`
and `GENERATED.md` pairs are bound receipts/excerpts, not duplicate backends.
Start with `examples/hello_counter/` or `examples/counter/`; the
[example catalog](https://github.com/PTO-ISA/pyCircuit/blob/main/examples/README.md)
provides the complete design navigation.

## Documentation and records

| Folder | Purpose |
| --- | --- |
| `docs/getting-started/` | Install, author and run the first design. |
| `docs/reference/` | Current language and hardware contracts. |
| `docs/architecture/` | Compiler and simulation structure. |
| `docs/development/` | Contribution, ownership, build, test and review guidance. |
| `docs/figures/` | Project logo and social-preview artwork. |
| `docs/legal/` | Retained licensing provenance. |
| `docs/rfcs/contracts/` | Preserved proposals and exact approval records; support is determined by current implementation and evidence, not a proposal alone. |
| `docs/gates/` | One canonical [local evidence policy](../gates/README.md). |

Local task packets, review notes and candidate logs belong together under
`docs/gates/logs/<run-id>/`. Historical `docs/work-items/` and `docs/reviews/`
records remain ignored; their duplicated tracked README policy has been
consolidated into the gates guide. Unfinished migration is tracked in
[issue #272](https://github.com/PTO-ISA/pyCircuit/issues/272), while language and
performance work have separate trackers linked there.

## Build entrypoints and local outputs

- Root `CMakeLists.txt` owns build, install and package targets.
- Root `CMakePresets.json` provides current native build presets.
- `flows/scripts/pyc` and `flows/scripts/pyc.ps1` configure/build/install through
  that same graph. The API and example scripts select existing test tiers.
- `pyproject.toml` owns the Python package, CLI and Python development checks.

The obsolete root Makefile and compiler-local presets have been removed;
there is no fallback build route for retired executables.

`.pycircuit_out/`, out-of-source builds, `_build/`, virtual environments, test
caches and `.omx/` are local generated/tool state, not tracked source folders.
Their presence in a filesystem browser does not add a compiler or product API.
Clean them only when no active build or investigation needs them; never use
another checkout's binaries as verification evidence.
