# ACC-only SDK release contract

This document defines the installable pyCircuit 6 SDK after the structural-
identity hard break. The SDK exposes one Agentic Circuit compiler flow.

## Identity

The release identity is the tuple in `packaging/sdk/version-map.json`:

- product and distribution versions;
- ACIR/ACPy schema versions and runtime ABI;
- target platform profile;
- the full 40-character Git `source_revision` peeled from the annotated tag.

The source revision is the sole release pin. ACIR, specialization, generated
source, SDK files, archives, wheels, dependencies, and release records carry no
byte-derived identity.

## Installed compiler flow

The only Agentic Circuit compilation interface is:

```text
acc.py -c <module.py> -o <module.ac>
acc -c <module.ac> -emit-cpp -o <module.cpp>
acc -c <module.ac> -emit-cpp-bundle -o <generated-dir>
acc -c <module.ac> -emit-verilog -o <module.v>
```

`acc.py` uses the supported Python frontend and publishes one verified ACIR
system. Native `acc` reparses and verifies that structure before lowering.
No producer-provided byte identity is trusted or recomputed.

The bundle form requires an absent or empty destination and publishes it as one
transaction. It never updates an existing generated directory in place and
therefore needs no ownership manifest, stale-file reconciliation, or persistent
content-addressed cache.

## Specialization and reuse

Specialization identity is structural:

```text
(MLIR definition symbol, ordered typed static arguments)
```

The compiler compares that structure directly. Generated C++ class names spell
the definition and any distinguishing parameters according to
`docs/reference/name-mangling.md`. Repeated instances of the same structure
reuse one class while retaining independent runtime state.

## SDK inventory

Each platform archive contains `share/pycircuit/sdk-manifest.json`. Despite the
historical filename, it is only a closed inventory of relative paths and kinds,
plus the release tuple, compiler identity, platform profile, ABI versions, and
runtime-dependency classification. It contains no file-size or nested producer
identity.

The external copy of that inventory must match the embedded copy byte for byte.
Consumers verify:

- exact inventory closure and safe relative paths;
- expected tools (`acc.py`, `acc`, `pycc`) and libraries;
- native dependency closure and relocatability;
- absence of producer-absolute paths;
- the exact Git `source_revision` and platform tuple;
- compilation and execution of an ACC-generated DUT.

## CMake components

`Runtime` provides `AgenticCircuit::Gfsim`, generated-model headers, and the
runtime link interface without requiring LLVM or MLIR. `CompilerDev` provides
compiler development targets and requires the pinned LLVM/MLIR toolchain.

```cmake
find_package(AgenticCircuit 0.1.0 EXACT CONFIG REQUIRED COMPONENTS Runtime)
target_link_libraries(gfsim-pyc PRIVATE AgenticCircuit::Gfsim)
find_package(AgenticCircuit 0.1.0 EXACT CONFIG REQUIRED COMPONENTS CompilerDev)
```

The runtime ABI continues to expose `agentic_model_query_v1` and the existing
`create -> configure -> reset -> step` lifecycle with its fixed 24-byte result.
Generated C++ is otherwise source-level output, not a stable cross-
release C++ ABI.

## Release inventory

The release index and consumer lock name assets by stable release URL, platform,
version, and source revision. They do not contain per-asset byte identities.
Release transport integrity belongs to
the package host and signing/attestation system, outside the compiler language
contract.

The supported profiles remain:

| Platform ID | Builder | Minimum runtime | Host C++ ABI |
| --- | --- | --- | --- |
| `linux-x86_64` | `ubuntu-24.04` | Ubuntu 24.04, glibc 2.39 | C++20, libstdc++ CXX11 ABI |
| `macos-arm64` | `macos-15` | macOS 15 | C++20, Apple libc++ |
| `windows-x86_64` | `windows-2022` | Windows Server 2022 | C++20, MSVC v143 |

The release contains exactly one `pycircuit-hisi` wheel per supported platform.
That wheel carries both frontends (`pycircuit`, `agentic_circuit`,
`_pycircuit_semantics`) and both compilers (`pycc`, `acc`) as console scripts, so
there is no second distribution to install and no universal wheel to match.

## Required verification

- Every public schema and example rejects byte-derived identity fields.
- A clean SDK archive has exact inventory closure and relocatable native
  dependencies on all supported platforms.
- The installed `acc.py -> .ac -> acc` flow emits, compiles, and runs a C++ DUT.
- `acc -emit-verilog` uses canonical PYC/`pycc` lowering.
- Repeating a compile may be compared directly in the test process, but no
  persistent content identity or cache record is published.
