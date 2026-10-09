<p align="center">
  <img src="docs/figures/pycircuit-logo.png" alt="pyCircuit" width="380">
</p>

# pyCircuit

**Describe hardware in Python. Compile once. Simulate in C++ or Verilog.**

<p align="center">
  <a href="https://github.com/PTO-ISA/pyCircuit/actions/workflows/ci.yml"><img src="https://github.com/PTO-ISA/pyCircuit/actions/workflows/ci.yml/badge.svg?branch=main" alt="CI"></a>
  <a href="https://github.com/PTO-ISA/pyCircuit/actions/workflows/release.yml"><img src="https://github.com/PTO-ISA/pyCircuit/actions/workflows/release.yml/badge.svg" alt="Release"></a>
  <a href="https://github.com/PTO-ISA/pyCircuit/releases/latest"><img src="https://img.shields.io/github/v/release/PTO-ISA/pyCircuit?display_name=tag&amp;sort=semver" alt="Latest release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/PTO-ISA/pyCircuit" alt="BSD 3-Clause license"></a>
  <a href="docs/getting-started/installation.md"><img src="https://img.shields.io/badge/Python-3.11%2B-3776AB?logo=python&amp;logoColor=white" alt="Python 3.11 or later"></a>
  <a href="toolchains/llvm.lock.json"><img src="https://img.shields.io/badge/LLVM%2FMLIR-22.1.8-5C2D91" alt="LLVM and MLIR 22.1.8"></a>
</p>

[Get started](docs/getting-started/quickstart.md) ·
[Language reference](docs/reference/language.md) ·
[Examples](examples/README.md) ·
[Repository map](docs/development/repository-layout.md) ·
[Contribute](CONTRIBUTING.md)

pyCircuit is a hardware programming language and compiler built on Python syntax
and MLIR. Typed modules, persistent state and stateless rules describe the design.
The compiler checks types, dependencies and effects, then emits C++ simulation
models and Verilog from the same verified hardware IR.

Use it to build reusable hardware modules, compose stateful designs, and run
closed simulation systems with compiler-generated drivers. Python supplies the
authoring syntax; capture parses the source without executing the design.

## What you get

- **Hardware semantics in familiar syntax.** Fixed-width values, records, Tables,
  rules and module composition, with explicit admission and diagnostics.
- **Two backends, one verified design.** GFSIM C++ models and Verilog consume the
  same final artifact, with source ownership preserved across compilation units.
- **Executable systems.** A closed `@system` describes the DUT, stimulus and
  checks. The compiler generates native and Verilator simulation harnesses.
- **Atomic simulation steps.** Rules read the old state. Whole-system checks
  precede commit; a failed epoch commits neither state nor clock history and
  publishes no source observations.
- **A separate Runtime package.** Generated C++ consumers use the Runtime CMake
  component without installing LLVM or the compiler development package.

The checkout identifies itself as **pyCircuit 6.1**. The language is under active
development: general parameter-dependent elaboration, broader clock scheduling,
wide/four-state observation transport and some collection forms remain limited.
The [language reference](docs/reference/language.md) defines supported constructs;
[known limitations](docs/development/known-limitations.md) records the boundaries.

## A complete counter

```python
from pycircuit import bits, log, rule, system


@rule
def increment(count):
    log("info", "count", count)
    count = count + 1


@system
def HelloCounter():
    count: bits[8] = 0
    increment(count)
```

`count` is eight-bit persistent storage. The rule observes its current value and
proposes an increment modulo 256. The compiler infers storage, clock/reset
connections and write enables, and generates the simulation harness.

From a checkout with an installed toolchain:

```sh
pycircuit run examples/hello_counter --target cpp --cycles 5
pycircuit run examples/hello_counter --target verilog --cycles 5
```

Each cycle contains low and high sampling epochs, so both runs log
`0, 0, 1, 1, 2, 2, 3, 3, 4, 4`. Generated files and simulator builds stay under
`.pycircuit_out/run/hello_counter/`. Verilog execution requires Verilator.
Use `--toolchain /absolute/path/to/install` when selecting an external toolchain.

See the [quickstart](docs/getting-started/quickstart.md) for explicit compile/link/
emit commands and the [tutorial](docs/getting-started/tutorial.md) for inspecting
and running the generated artifacts.

## Build from source

| Task | Requirements |
| --- | --- |
| Build the compiler | Python 3.11+, CMake 3.25+, Ninja, a C++20 compiler, LLVM and MLIR **22.1.8** |
| Run generated C++ | C++20 toolchain, CMake/Ninja and pyCircuit Runtime |
| Run generated Verilog systems | Installed pyCircuit compiler, Verilator, CMake/Ninja and a C++20 toolchain |

Install LLVM/MLIR first. The build helper discovers `llvm-config-22` or a matching
`llvm-config`; alternatively set `LLVM_CONFIG`, or both `LLVM_DIR` and `MLIR_DIR`,
to that installation. Installing the Python package alone does not install the
native compiler.

```sh
git clone https://github.com/PTO-ISA/pyCircuit.git
cd pyCircuit
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e .
PYC_BUILD_TESTING=OFF bash flows/scripts/pyc build

export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
export PATH="$PYC_TOOLCHAIN_ROOT/bin:$PATH"
pycircuit run examples/hello_counter --target cpp --cycles 5
```

These commands use a POSIX shell. The [installation guide](docs/getting-started/installation.md)
covers CMake build options, Runtime-only installations and package consumers.
For prebuilt wheels, check the assets and platform support of the specific
[release](https://github.com/PTO-ISA/pyCircuit/releases).

## One compiler flow

```text
Python modules and systems
          │ syntax capture
          ▼
MLIR type, dependency and effect analysis
          │ hardware lowering and verification
          ▼
Independent source units ── explicit link closure ── verified final IR
                                                        │
                                               ┌────────┴────────┐
                                               ▼                 ▼
                                          GFSIM C++           Verilog
```

| Command | Purpose |
| --- | --- |
| `pycircuit compile` | Compile one source and publish its body, interface and dependencies. |
| `pycircuit link` | Check the complete explicit source-unit closure and select the design root. |
| `pycircuit emit` | Emit C++ or Verilog from the verified final artifact. |
| `pycircuit run` | Build and execute a closed system through those same public stages. |

`@module` defines reusable hardware, `@rule` defines stateless computations, and
`@system` defines a closed simulation composition. The current system path owns
its generated clock/reset scaffolding; ordinary module drivers can provide
explicit physical inputs. See [system execution](docs/architecture/system-execution.md)
for sampling, reset, observation and failure semantics.

## Explore and contribute

| Start with | For |
| --- | --- |
| [Repository map](docs/development/repository-layout.md) | Folder responsibilities, source boundaries and the right place for a change. |
| [Example catalog](examples/README.md) | Counters, pipelines, queues, tables and larger designs, with their verification owners. |
| [Python language](docs/reference/language.md) | Types, source constructs, composition and rejection boundaries. |
| [Compiler architecture](docs/architecture/compiler-pipeline.md) | Capture, MLIR analyses, source publication and code generation. |
| [Build and test workflow](docs/development/testing-and-gates.md) | Bounded gates, native tests and nightly coverage. |
| [Contribution guide](CONTRIBUTING.md) | Scope, contracts, review and reproducible changes. |

For compiler development, enable `PYC_BUILD_TESTING=ON` and install the native
test dependencies, including GoogleTest and lit. With that testing build
installed, run the bounded checks from the repository root:

```sh
bash flows/scripts/run_api_tests.sh --tier gate
bash flows/scripts/run_examples.sh --tier gate
```

For a nondefault build, set `PYC_BUILD_DIR` and `PYC_TOOLCHAIN_ROOT` as described
in the testing guide. PR CI checks Python, publication, repository hygiene and
documentation; native, example and release evidence have their own documented
entrypoints. A green CI badge does not imply full backend or platform coverage.

Unfinished historical example migration and full-catalog validation are tracked
in [issue #272](https://github.com/PTO-ISA/pyCircuit/issues/272). A catalog entry
or a removed example is not a claim of completed verification. Retired frontends
and compiler aliases are not compatibility paths.

## License

pyCircuit is licensed under [BSD-3-Clause](LICENSE).
