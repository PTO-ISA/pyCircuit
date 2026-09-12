# pyCircuit 6

<p align="center">
  <strong>Python-first hardware construction and architecture modeling, backed by MLIR, deterministic C++ simulation, and Verilog generation.</strong>
</p>

<p align="center">
  <a href="https://github.com/PTO-ISA/pyCircuit/actions/workflows/ci.yml"><img src="https://github.com/PTO-ISA/pyCircuit/actions/workflows/ci.yml/badge.svg?branch=main" alt="CI"></a>
  <a href="https://github.com/PTO-ISA/pyCircuit/actions/workflows/release.yml"><img src="https://github.com/PTO-ISA/pyCircuit/actions/workflows/release.yml/badge.svg" alt="Release"></a>
  <a href="https://github.com/PTO-ISA/pyCircuit/releases/latest"><img src="https://img.shields.io/github/v/release/PTO-ISA/pyCircuit?display_name=tag&sort=semver" alt="Latest release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/PTO-ISA/pyCircuit" alt="BSD 3-Clause license"></a>
  <a href="docs/getting-started/installation.md"><img src="https://img.shields.io/badge/Python-3.10%2B-3776AB?logo=python&logoColor=white" alt="Python 3.10 or later"></a>
  <a href="toolchains/agentic-circuit/llvm.lock.json"><img src="https://img.shields.io/badge/LLVM%2FMLIR-22.1.8-5C2D91" alt="LLVM and MLIR 22.1.8"></a>
</p>

pyCircuit provides two complementary Python frontends in one versioned
toolchain:

- **`pycircuit`** constructs cycle-aware, synthesizable hardware from signals,
  state, hierarchy, memories, and explicit logical cycles.
- **`agentic_circuit`** models architecture-level processes, queues, resources,
  scheduling, and committed state through ACPy and ACIR.

Both paths converge on verified PYC MLIR when generating hardware. C++ and
Verilog therefore share one semantic contract rather than separate handwritten
implementations.

## Highlights

- Cycle-aware signals with automatic pipeline balancing
- Structural and cycle-aware authoring on one verified PYC representation
- Preserved module hierarchy and deterministic generated artifacts
- Exact-width values, typed structures, queues, tables, and memories
- C++ cycle simulation, gfsim architecture simulation, and Verilog generation
- Focused pull-request gates plus reproducible release closure

## Choose a frontend

| You want to describe | Install | Import | Primary flow |
| --- | --- | --- | --- |
| Ports, signals, registers, memories, pipelines, and synthesizable hardware | `pycircuit-hisi` | `pycircuit` | Python → PYC → `pycc` → C++ / Verilog |
| Processes, queues, resources, scheduling, and architecture state | `agentic-circuit` | `agentic_circuit` | Python → ACPy 0.5 → ACIR → ACSim/gfsim or PYC |

Read [Choose a Frontend](docs/getting-started/choose-a-frontend.md) for the
supported authoring boundaries and examples.

## Quick start

The integrated development setup requires Python 3.11 or later, CMake, Ninja,
and LLVM/MLIR 22.1.8. pyCircuit-only frontend use supports Python 3.10 or later.

```bash
git clone https://github.com/PTO-ISA/pyCircuit.git
cd pyCircuit

python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e "python/semantic-core"
python -m pip install -e ".[dev,docs]"

bash flows/scripts/pyc build
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
```

Build the counter example with the C++ and Verilog backends:

```bash
PYTHONPATH=python/pycircuit/src \
python -m pycircuit.cli build \
  examples/pycircuit/basics/counter/tb_counter.py \
  --out-dir .pycircuit_out/quickstart/counter \
  --target both \
  --jobs 8
```

Add the architecture-modeling frontend when needed:

```bash
python -m pip install -e "python/agentic-circuit[test]"
agentic-circuit --help
```

Continue with the [Quickstart](docs/getting-started/quickstart.md) or the
[complete installation guide](docs/getting-started/installation.md).

## How the toolchain fits together

```text
agentic_circuit frontend -> ACPy 0.5 -> ACIR
                                         |-> ACSim -> gfsim
                                         `-> PYC -> pycc -> pyc6 C++ / Verilog

pycircuit frontend -> Cycle-Aware Signal -> PYC -> pycc -> pyc6 C++ / Verilog
```

ACIR remains an architecture-level dialect; PYC remains the shared hardware
contract. The `pycircuit` and `agentic_circuit` Python namespaces are separate
and are not compatibility aliases.

## Documentation

| Start here | Purpose |
| --- | --- |
| [Getting Started](docs/getting-started/index.md) | Install the toolchain and run the first design |
| [pyCircuit 6 Tutorial](docs/getting-started/tutorial.md) | Learn cycle-aware authoring and testbenches |
| [Language and API Reference](docs/reference/index.md) | Look up syntax, APIs, diagnostics, primitives, and PYC IR |
| [Architecture](docs/architecture/overview.md) | Understand frontends, compiler stages, runtimes, and backends |
| [Agentic Circuit and ACIR](docs/acir/index.md) | Learn ACPy, ACIR/ACSim, QueueGraph, and gfsim |
| [Development Guide](docs/development/index.md) | Build, test, contribute, and prepare pull requests |

## Repository layout

The tree is organized by responsibility:

```text
python/       Python distributions and shared semantics
compiler/     PYC and ACIR/ACSim dialects, passes, and generators
library/      Stable pyCircuit C++ runtime and Verilog implementations
simulator/    gfsim architecture-modeling runtime
docs/         User, architecture, reference, and contributor documentation
examples/     Small supported examples
benchmarks/   Performance-only workloads and harnesses
tests/        Unit, system, integration, MLIR, C++, Verilog, and golden tests
tools/        User-facing and product-maintenance utilities
flows/        Build, CI, gate, and release orchestration
schemas/      Machine-readable contracts, inventories, and registries
packaging/    SDK, archive, and wheel assembly
toolchains/   Pinned compiler and dependency identities
```

See [Repository Layout](docs/development/repository-layout.md) for the complete
ownership map, including `.github/`, `cmake/`, and `third_party/`. Complete CPU,
NPU, accelerator, SoC, board, ISA, and product-specific testbench sources live
in their owning consumer repositories.

## Validate a change

Run the lightweight repository checks first:

```bash
pre-commit run --all-files
pytest tests/unit -m unit
python tools/agentic-circuit/check-contracts.py
mkdocs build --strict
```

Native compiler or runtime changes also require the narrowest affected MLIR,
C++, simulation, or backend test. The full release matrix runs through the
release workflow rather than every pull request.

See [Testing and Gates](docs/development/testing-and-gates.md) for the exact
change-to-gate mapping.

## Project status

[`PTO-ISA/pyCircuit`](https://github.com/PTO-ISA/pyCircuit) is the canonical
source, issue tracker, and release authority. The latest published release is
available from [GitHub Releases](https://github.com/PTO-ISA/pyCircuit/releases).
The distribution name is `pycircuit-hisi`; the Python import remains
`pycircuit`.

The former standalone Agentic Circuit repository is archived provenance. Its
consolidation record is preserved in the
[historical repository record](docs/acir/spec/refs/history.md), not as an active
development or compatibility path.

## Contributing and security

- [Contributing guide](CONTRIBUTING.md)
- [Development workflow](docs/development/contributing-workflow.md)
- [Review and merge requirements](docs/development/review-and-merge.md)
- [Security policy](SECURITY.md)
- [Code of conduct](CODE_OF_CONDUCT.md)

## License

pyCircuit and the integrated Agentic Circuit sources are licensed under the
[BSD 3-Clause License](LICENSE).
