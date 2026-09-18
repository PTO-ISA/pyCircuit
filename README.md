<p align="center">
  <img src="docs/figures/pycircuit-logo.png" alt="pyCircuit" width="380">
</p>

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

---

pyCircuit provides two complementary Python frontends in one versioned
toolchain:

- **`pycircuit`** constructs cycle-aware, synthesizable hardware from signals,
  state, hierarchy, memories, and explicit logical cycles.
- **`agentic_circuit`** models architecture-level processes, queues, resources,
  scheduling, and committed state through ACPy and ACIR.

Both paths converge on verified PYC MLIR when generating hardware. C++ and
Verilog therefore share one semantic contract, rather than separate handwritten
implementations.

## Highlights

- **Cycle-aware by construction.** Signal provenance tracks logical cycles, and
  automatic pipeline balancing lowers to explicit PYC MLIR (Decision 0148).
- **One verified contract.** Structural and cycle-aware authoring reach the same
  verified PYC representation; semantics live in the dialect, passes, and
  verifiers rather than in one backend.
- **Deterministic output.** Preserved module hierarchy and deterministic
  generated artifacts.
- **Rich data model.** Exact-width values, typed structures, queues, tables, and
  memories.
- **Multiple backends.** C++ cycle simulation, gfsim architecture simulation,
  and Verilog generation from one design.
- **Reviewable change control.** Focused pull-request gates plus a reproducible
  release closure.

## Choose a frontend

| You want to describe | Install | Import | Primary flow |
| --- | --- | --- | --- |
| Ports, signals, registers, memories, pipelines, and synthesizable hardware | `pycircuit-hisi` | `pycircuit` | Python → PYC → `pycc` → C++ / Verilog |
| Processes, queues, resources, scheduling, and architecture state | `agentic-circuit` | `agentic_circuit` | Python → `acc.py` → verified ACIR → `acc` → C++ / bundle / Verilog |

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

## Write your first circuit

Each frontend has one authoring entry point, and both lower to verified PYC
before any backend runs.

### Cycle-aware hardware with `pycircuit`

`CycleAwareSignal` is the primary authoring model. Compute the next value in the
current logical cycle, call `domain.next()`, then commit the assignment; the
frontend inserts delay registers wherever cycles must be balanced.

```python
from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    build_cycle_aware,
    cas,
    mux,
    wire_of,
)


def build(m: CycleAwareCircuit, domain: CycleAwareDomain, width: int = 8) -> None:
    enable = cas(domain, m.input("enable", width=1), cycle=0)
    count = domain.signal(width=width, reset_value=0, name="count")

    m.output("count", wire_of(count))

    # Compute next at cycle 0, then commit after domain.next().
    count_next = mux(enable, count + 1, count)
    domain.next()
    count <<= count_next


build.__pycircuit_name__ = "counter"

print(build_cycle_aware(build, name="counter", width=8).emit_mlir())
```

The full design, testbench, and parameters live in
[`examples/pycircuit/basics/counter`](examples/pycircuit/basics/counter), and the
[pyCircuit 6 Tutorial](docs/getting-started/tutorial.md) covers testbenches,
hierarchy, memories, and multi-cycle pipelines.

### Transactional architecture with `agentic_circuit`

Use Agentic Circuit when a design is better described as typed data moving
through queues and atomic rules. Availability, backpressure, reservations,
arbitration, and commit are compiler responsibilities, so the Python describes
intent rather than hand-built handshakes.

```python
import agentic_circuit as ac


@ac.struct
class Entry:
    index: ac.u2
    value: ac.u8


def increment(entry: Entry) -> Entry:
    return entry.with_fields(value=entry.value + 1)


@ac.rule
def install(entries, incoming):
    old = entries[incoming.index]
    entries[incoming.index] = increment(incoming)
    return old


@ac.system
def transaction_pipeline(incoming: Entry) -> Entry:
    entries = ac.table[4, Entry](init=0)
    outgoing = install(entries, incoming)
    return outgoing
```

Every `@ac.rule` is one schedulable atomic transition. Here the Table
replacement and the Queue transfers in `install` prepare together and publish at
the same tick edge, so backpressure or a state conflict leaves the committed
image unchanged; no reservation, check, or prepare/publish step appears in the
Python.

Runnable state examples, including explicit `ac.source()` and `ac.sink()`
capture forms, multi-rule ROB scheduling, and slot ownership, live in
[`examples/agentic-circuit/state`](examples/agentic-circuit/state). The
[Agentic Circuit and ACIR](docs/acir/index.md) documentation covers ACPy,
verified ACIR, ACC, QueueGraph, and gfsim, and the
[Agent Frontend Guide](docs/development/agent-frontend-guide.md) states the
authoring rules this example follows.

## How the toolchain fits together

```text
agentic_circuit frontend -> acc.py -> verified ACIR -> acc
                                                   |-> gfsim C++ / bundle
                                                   `-> PYC -> pycc -> Verilog

pycircuit frontend -> Cycle-Aware Signal -> PYC -> pycc -> pyc6 C++ / Verilog
```

ACIR remains an architecture-level dialect; PYC remains the shared hardware
contract. The `pycircuit` and `agentic_circuit` Python namespaces are separate
and are not compatibility aliases.

## Documentation

| Start here | Purpose |
| --- | --- |
| [Getting Started](docs/getting-started/index.md) | Install the toolchain and run the first design |
| [Choose a Frontend](docs/getting-started/choose-a-frontend.md) | Select between `pycircuit` and `agentic_circuit` |
| [pyCircuit 6 Tutorial](docs/getting-started/tutorial.md) | Learn cycle-aware authoring and testbenches |
| [Language and API Reference](docs/reference/index.md) | Look up syntax, APIs, diagnostics, primitives, and PYC IR |
| [Architecture](docs/architecture/overview.md) | Understand frontends, compiler stages, runtimes, and backends |
| [Agentic Circuit and ACIR](docs/acir/index.md) | Learn ACPy, verified ACIR, ACC, QueueGraph, and gfsim |
| [Development Guide](docs/development/index.md) | Build, test, contribute, and prepare pull requests |
| [Agent Frontend Guide](docs/development/agent-frontend-guide.md) | Choose and apply a Pythonic authoring model for complex circuits |

## Repository layout

The tree is organized by responsibility:

```text
python/       Python distributions and shared semantics
compiler/     PYC and ACIR dialects, passes, ACC, and generators
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
