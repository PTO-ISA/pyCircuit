# Installation

The active interface is one Python capture/driver package and one CMake Runtime
component. The compiler developer tools are optional for consumers of generated
C++ models.

## Requirements

| Use | Requirements |
| --- | --- |
| Author and compile Python sources | Python 3.11+, pyCircuit Python package, native compiler installed from a CompilerDev build |
| Build and run generated C++ | CMake 3.25+, Ninja, C++20 compiler, installed Runtime |
| Develop the compiler | Above, plus LLVM and MLIR exactly 22.1.8 |
| Emit Verilog | CompilerDev build; downstream Verilog simulation tools are separate |

Typed modules, state variables and rules use one Python frontend. Generated
models expose typed C++ DUT access; host drivers supply sampling inputs.
See the [language reference](../reference/language.md) for exact admission and
[known limitations](../development/known-limitations.md) for unfinished work.

## Build and install from source

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e .

cmake -S . -B .pycircuit_out/build -G Ninja \
  -DCMAKE_INSTALL_PREFIX="$PWD/.pycircuit_out/install" \
  -DPYC_BUILD_COMPILER_DEV=ON \
  -DPYC_BUILD_TESTING=OFF \
  -DPYC_BUILD_RUNTIME_LIB=ON
cmake --build .pycircuit_out/build
cmake --install .pycircuit_out/build
export PATH="$PWD/.pycircuit_out/install/bin:$PATH"
pycircuit --help
```

`PYC_BUILD_COMPILER_DEV` enables the native source compiler and requires exact
LLVM/MLIR 22.1.8. `PYC_BUILD_TESTING` controls native compiler test targets.
`PYC_BUILD_RUNTIME_LIB` controls the generated-model runtime. The install prefix
contains the `pycircuit` driver, Python capture package, CMake package metadata,
and enabled native libraries.

## Runtime-only install

A consumer building generated C++ needs no compiler developer package or LLVM.
Configure a Runtime-only install with:

```bash
cmake -S . -B .pycircuit_out/runtime-build -G Ninja \
  -DCMAKE_INSTALL_PREFIX="$PWD/.pycircuit_out/runtime-install" \
  -DPYC_BUILD_COMPILER_DEV=OFF \
  -DPYC_BUILD_TESTING=OFF \
  -DPYC_BUILD_RUNTIME_LIB=ON \
  -DPYC_INSTALL_PYTHON=OFF
cmake --build .pycircuit_out/runtime-build
cmake --install .pycircuit_out/runtime-build
```

A CMake consumer links the one exported Runtime target:

```cmake
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
add_executable(model main.cpp)
target_link_libraries(model PRIVATE pycircuit::pyc6_runtime)
```

The exported target is `pycircuit::pyc6_runtime`, backed by
`libpyc6_runtime`. It carries runtime headers and libraries only and does not
search for LLVM/MLIR.

## Compiler developer component

Compiler authors can consume the exported native compiler targets using:

```cmake
find_package(pycircuit CONFIG REQUIRED COMPONENTS CompilerDev)
```

This component requires LLVM and MLIR `22.1.8` exactly. The Runtime and
CompilerDev components are independently selectable; no retired PYC package,
forwarding target, or fallback compiler is provided.

## Python package

From a checkout, `python -m pip install -e .` installs the `pycircuit` package
and its driver. The public commands are `pycircuit compile`, `pycircuit link`,
and `pycircuit emit`. Historical commands including `acc.py`, `acc`, `pycc`,
and `agentic-circuit` are retired and are not compatibility aliases.

For a released wheel, use the project release page and install the wheel matching
your platform. Release availability and platform support are properties of the
specific release. The source checkout identifies itself as pyCircuit 6.1.
