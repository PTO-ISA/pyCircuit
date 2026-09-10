# Stable SDK release contract

This document defines the first installable pyCircuit SDK contract for generic
external model consumers. It is the source of truth for issue #61.

The contract is accepted before implementation so each implementation PR has a
fixed boundary. A `v6.0.0` release remains blocked until every required command,
artifact, ABI, relocation check, and post-publish check in this document passes.

## Identity and compatibility

The candidate release tuple is stored in
`packaging/sdk/version-map.json`.

| Identity | Required value |
| --- | --- |
| Product and SDK | `6.0.0` |
| Candidate tag | `v6.0.0` |
| `pycircuit-hisi` | `6.0.0` |
| `pycircuit-semantic-core` | `6.0.0` |
| `agentic-circuit` | `0.1.0` |
| ACPy and ACIR contract epoch | `0.5` |
| SDK manifest schema | `1` |
| Release index schema | `1` |
| Model plan schema | `1` |
| Model manifest schema | `1` |
| Generator and runtime ABI | `1` |
| Consumer lock schema | `1` |

The final source revision is the 40-character commit obtained by peeling the
annotated release tag. Every platform manifest, release index, model plan,
model manifest, consumer lock, release note, and post-download report MUST
contain that exact revision.
A workflow run ID, branch name, moving `main` reference, or temporary artifact
ID is not a source identity.

Compatibility is exact across this entire tuple. Version numbers in different
Python distributions need not match, but their mapping MUST match the version
map and the SDK manifest. No cross-release C++ ABI, model-plan ABI, or generated
runtime ABI compatibility is implied.

## Supported platforms

The first release supports exactly these build and consumption profiles:

| Platform ID | Builder | Minimum runtime | Python | Host C++ ABI |
| --- | --- | --- | --- | --- |
| `linux-x86_64` | `ubuntu-24.04`, x86_64 | Ubuntu 24.04, glibc 2.39 | 3.11 | C++20, libstdc++ CXX11 ABI |
| `macos-arm64` | `macos-15`, arm64 | macOS 15 | 3.11 | C++20, Apple libc++ |

The release job MUST fail when the runner OS or architecture differs from its
profile. The manifest records the actual compiler identity. A package built on
one profile does not claim support for another Linux distribution, older libc,
macOS x86_64, Windows, or another Python minor.

RTTI and exceptions are build properties of each exported target. The SDK MUST
record them and MUST compile the external consumer with compatible settings.
The Runtime component may depend on documented system C/C++ libraries. Every
other native dependency is bundled and resolved relative to the installed SDK.

## Artifact set and install layout

Each platform publishes one archive named:

```text
pycircuit-sdk-6.0.0-<platform-id>.tar.gz
```

Each archive embeds one platform manifest at
`share/pycircuit/sdk-manifest.json`. The same bytes are attached with the unique
name `pycircuit-sdk-6.0.0-<platform-id>.manifest.json`. A separate
`pycircuit-sdk-6.0.0-release-index.json` names both archives, both platform
manifests, the exact four-wheel set, license material, and release notes. The
set contains one `pycircuit-hisi` wheel for each supported platform plus the
universal `pycircuit-semantic-core` and `agentic-circuit` wheels.

`SHA256SUMS` covers every attached file including the release index, but does
not contain a checksum for itself. A platform manifest lists and hashes the
installed files inside its archive except its own embedded manifest; it does not
record the enclosing archive hash. This topology has no self-referential hash.

The SDK archive has this logical layout:

```text
bin/
  agentic-circuit
  acir-opt
  acir-queue-plan
  acir-queue-cxxgen
  pycc
  pyc-opt
include/
  gfsim/model_api.h
  gfsim/**
  pycircuit/**
lib/
  cmake/AgenticCircuit/**
  cmake/pycircuit/**
  libgfsim.*
  libpyc6_runtime.*
python/
  wheelhouse/*.whl
share/pycircuit/
  schemas/**
  sdk-manifest.json
  licenses/**
```

Additional files are allowed only when the SDK manifest classifies them. The
archive MUST NOT contain an absolute producer source path, build path, Homebrew
path, runner tool-cache path, or unresolved symlink outside the archive.

## CMake components

The installed package exposes two explicit components.

### Runtime

```cmake
find_package(AgenticCircuit 0.1.0 EXACT CONFIG REQUIRED COMPONENTS Runtime)
target_compile_features(gfsim-pyc PRIVATE cxx_std_20)
target_link_libraries(gfsim-pyc PRIVATE AgenticCircuit::Gfsim)
```

`Runtime` provides the generated model ABI, gfsim headers, gfsim runtime, and
complete transitive runtime link interface. Resolving this component MUST NOT
call `find_dependency(LLVM)` or `find_dependency(MLIR)` and MUST succeed when
LLVM and MLIR development packages are hidden.

The Runtime header tree and `AgenticCircuit::Gfsim` contain no LLVM, MLIR, ACIR,
consumer payload, or workload-trace dependency. External runtime consumers link
only Gfsim.

### CompilerDev

```cmake
find_package(AgenticCircuit 0.1.0 EXACT CONFIG REQUIRED COMPONENTS CompilerDev)
```

`CompilerDev` exposes dialect, pass, analysis, and compiler-development targets.
It requires LLVM and MLIR `22.1.8` exactly. Requesting both components returns
the union. Omitting components preserves the CompilerDev behavior. Unknown
components fail with an actionable diagnostic that lists `Runtime` and
`CompilerDev`.

`pycircuit` runtime targets remain available through their existing package.
A configuration with zlib trace support MUST express a relocatable conditional
`find_dependency(ZLIB)`; a build without zlib MUST NOT acquire that dependency.

## Installed model commands

The public installed CLI contains two commands.

```text
agentic-circuit model plan
  --sdk-root <installed-sdk-prefix>
  --source-root <consumer-source-root>
  --entry <importable-module:top>
  --config <model.toml>
  --out-dir <plan-dir>

agentic-circuit model emit-cpp
  --sdk-root <installed-sdk-prefix>
  --plan <plan-dir/model-plan.json>
  --out-dir <generated-dir>
  --manifest <generated-dir/model-manifest.json>
  --depfile <generated-dir/model.d>
```

`--sdk-root` is required and is the only SDK discovery mechanism. The commands
validate `<sdk-root>/share/pycircuit/sdk-manifest.json` before using absolute
tool and schema paths derived from that prefix. They do not search a pyCircuit
checkout, `.pycircuit_out`, an unrelated `PATH` entry, `PYTHONPATH`, or a second
SDK environment variable. CMake always passes its resolved package prefix.

### Plan

`model plan` captures the transitive Python source closure and static config,
lowers through the current frontend and shared MLIR verifier/pass pipeline, and
writes:

```toml
version = "1"

[static]
entries = 8
```

The config is a closed document with exactly `version = "1"` and one `[static]`
table. Its keys bind the `ac.const[...]` parameters of the system named after
the colon in `--entry`; values must be portable I-JSON values. A relative
`--config` path is resolved under `--source-root`, and the resolved file must
remain inside that root.

The entry uses exact `importable.module:system` syntax. The module resolves to
one Python file or package under `--source-root`; the system is specialized
through the existing QueueGraph JIT frontend. Local imports are captured before
execution and classified as entry, contract, or import inputs.

The command writes:

```text
model-plan.json
frozen.ac.mlir
queuegraph.json
model-sources.cmake
model.d
```

The plan conforms to
`schemas/agentic-circuit/model-plan.schema.json`. Logical source paths are
relative to `--source-root`; the depfile alone may contain local absolute paths.
The plan records each input and config hash, the Frozen ACIR and QueueGraph
identities and hashes, the exact SDK/source/ABI tuple, the selected entry and
specialization, required capabilities, the complete deterministic output file
list, the hashed CMake source fragment, and the depfile's logical path.

Version 1 predicts exactly these P3 generated files:

```text
include/generated/model.h
src/generated/model.cpp
src/generated/queuegraph.cpp
```

The plan's `model-sources.cmake` lists those relative paths and the fixed query
symbol and Runtime target. `model.d` names the local absolute source and config
dependencies; its bytes do not participate in the canonical plan identity.

`model plan` fails before publication when an import escapes the source root, a
source or config is invalid, the SDK tuple is inconsistent, a required tool is
missing, or the design uses an unsupported construct. It publishes the entire
plan directory atomically.

### Emit C++

`model emit-cpp` consumes the plan and verifies every recorded identity and
hash. It does not import consumer Python or reinterpret the design. It emits a
sorted multi-translation-unit source bundle that consumes the installed
versioned model ABI header, `model-sources.cmake`, the depfile, and a manifest conforming to
`schemas/agentic-circuit/model-manifest.schema.json`.

The canonical manifest records the depfile's logical path and never hashes the
depfile content. Plans, manifests, CMake fragments, and generated sources are
byte-identical across source roots and hash seeds; only local depfile content
may differ.

The source and config hashes are captured facts sealed by the canonical plan;
emit does not reopen those consumer files. It revalidates the plan schema and
SDK tuple, rehashes the plan-contained Frozen ACIR, QueueGraph, and CMake
fragment, rebuilds QueueGraph from Frozen ACIR, and requires byte equality
before invoking the installed C++ generator.

`model-sources.cmake` is declarative and sets exactly these variables:

- `AGENTIC_MODEL_GENERATED_SOURCES`: sorted generated `.cpp` paths relative to
  the generated root;
- `AGENTIC_MODEL_GENERATED_HEADERS`: sorted generated header paths relative to
  the generated root;
- `AGENTIC_MODEL_QUERY_SYMBOL`: `agentic_model_query_v1`;
- `AGENTIC_MODEL_RUNTIME_TARGET`: `AgenticCircuit::Gfsim`.

The fragment contains no command, target, generator expression, absolute path,
SDK lookup, or consumer-specific option. The consumer prepends its generated
root and owns the target definition.

The consumer builds those sources as a hidden-visibility shared library. Its
dynamic export list contains only `AGENTIC_MODEL_QUERY_SYMBOL`: use a GNU-style
version script plus `--exclude-libs,ALL` on Linux, or `-exported_symbol` on
macOS. Static Gfsim implementation symbols do not enter the plugin's public
dynamic symbol table.

One process holds an exclusive lock for an output root. Emission stages a closed
file set, validates it, then atomically publishes it. A failure keeps the prior
complete output. After success, files that were owned by the previous manifest
and are absent from the new manifest are deleted. No file outside that model
output root is deleted.

## Generated runtime ABI v1

Generated class layout is private. Consumers query one versioned C ABI table:

```cpp
extern "C" const AgenticModelApiV1 *agentic_model_query_v1();
```

The complete C ABI declaration is tracked at
`simulator/gfsim/include/gfsim/model_api.h`.
It fixes a 64-bit host ABI, structure sizes, integer status values, opaque model
handle, byte-buffer representation, epoch/step result layout, SDK identity
strings, and function signatures. The table has function pointers for:

- create and destroy;
- configure using a closed, consumer-neutral JSON document;
- reset;
- one simulation step with an explicit epoch/result;
- termination and failure status;
- canonical JSON statistics;

The exact structure is installed in `include/gfsim/model_api.h`. The only
exported symbol is `agentic_model_query_v1`; create, destroy, configure, reset,
step, statistics, and last-error operations are table
pointers. The query table is immutable for process lifetime. A model handle is
single-thread-owned and not reentrant. The caller serializes every call for one
handle. Returned buffers remain owned by the model and valid until the next
call on that handle or destroy. Input bytes remain caller-owned and need only
remain valid for the duration of the call.

Every function-table operation except the `void` destroy function returns a
stable integer status. `step` advances one scheduler epoch/delta and fills the
fixed 24-byte result with running, quiescent, terminated, or failed state. The
ABI does not expose a generated C++ class, STL container, MLIR/LLVM type,
consumer ELF loader, ISA decoder, or product-specific state.

The handle lifecycle is `create -> configure -> reset -> step`.
Calls in another order return `AGENTIC_MODEL_STATUS_V1_INVALID_STATE`; malformed
or noncanonical JSON and bad pointers or structure sizes return
`AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT`. Reset is allowed after ready or
terminated state and restores the same specialized model for deterministic
rerun.

`configure_json` accepts the closed empty config `{}` or this canonical runtime
limits document (shown with whitespace only for readability):

```json
{
  "deadlock_window": 8,
  "max_domain_cycles": {"cycle": 64},
  "max_ticks": 1024,
  "schema": "agentic-model-config",
  "version": "1"
}
```

Each limit may be `null`, and domain names are sorted. Unknown fields,
non-unsigned values, duplicate or unsorted domain names, and noncanonical bytes
are rejected.

Trace loading, trace cursors, workload-trace inputs, and observation/trace
exports are intentionally absent from the generated-model ABI. Consumers own
such adapters outside pyCircuit. Generated models link only
`AgenticCircuit::Gfsim`; they do not acquire LLVM, MLIR, or ACIR dependencies.

A model manifest names the exported query symbol and exact runtime ABI. A
consumer MUST reject a missing symbol or ABI mismatch before creating a model.

## Manifest contracts

The public schemas are:

- `sdk-version-map.schema.json`: candidate product/distribution mapping,
  contract versions, and the two exact platform profiles;
- `sdk-manifest.schema.json`: one installed platform, source, ABI,
  capabilities, dependencies, and installed file inventory without an
  enclosing-archive or self hash;
- `release-index.schema.json`: both archive/manifests, exact four-wheel map,
  supporting artifacts, hashes, sizes, and final URLs;
- `model-plan.schema.json`: verified source closure, config, IR identities,
  deterministic output plan, and required runtime;
- `model-manifest.schema.json`: plan identity, generated closed file set,
  CMake source fragment, depfile logical path, runtime target, and sole query
  symbol/function-table ABI;
- `consumer-lock.schema.json`: exact release asset, wheels, hashes, ABI, and
  capabilities selected by a consumer repository.

Unknown fields are rejected. All hashes use lowercase `sha256:<64-hex>`. Logical
paths are normalized relative paths without `..`. JSON is UTF-8 and canonical
with sorted object keys, no insignificant whitespace, and one trailing newline.

## Consumer build contract

A consumer performs these steps:

1. Verify its lock against `consumer-lock.schema.json`.
2. Download the named SDK archive and wheels, then verify every hash.
3. Install the wheels in a clean Python 3.11 environment.
4. Extract the SDK to an arbitrary prefix and pass that exact prefix through
   `--sdk-root`.
5. Run `model plan` during CMake configure with the same `--sdk-root`.
6. Include the generated `model-sources.cmake`.
7. Run `model emit-cpp` with the same `--sdk-root` as a build rule with declared
   outputs, byproducts, and depfile.
8. Compile every generated translation unit and link only
   `AgenticCircuit::Gfsim` plus consumer-owned adapters.

CMake uses `CMAKE_CONFIGURE_DEPENDS` for the transitive source list, config,
consumer lock, and SDK manifest. Source topology or SDK identity changes force a
new plan. Content-only changes rebuild emission through the depfile. Parallel
attempts for one output root serialize through the generator lock.

The consumer owns its input adapters, executable or image loading, memory
initialization, model configuration, test harness, and result comparison. The
SDK owns only generic frontend/compiler/runtime/model ABI behavior.

## Candidate and publication gates

The release graph is strictly ordered:

```text
full source closure
  -> build candidate SDK archives and wheels
  -> verify manifest and checksums
  -> install exact candidate bytes in clean environments
  -> move each SDK to an unrelated prefix
  -> run generic model generation and external harness
  -> publish the same bytes
  -> redownload published assets
  -> verify hashes and rerun smoke tests
  -> hand exact lock data to the consumer
```

Candidate validation runs on Linux x86_64 and macOS arm64. It hides the producer
source/build trees and LLVM/MLIR development package locations from Runtime
consumers. The generic fixture covers imported nominal contracts, const
specialization, multiple independent instances, admitted Table state,
heterogeneous optional outputs, backpressure, reset, incremental rebuild,
topology changes, concurrent generation, stale plan, wrong SDK, wrong ABI,
missing tool, and bounded Table PYC admission with out-of-profile rejection.

Every publish job depends on candidate acceptance. GitHub Release, GHCR, and
optional PyPI publication cannot run from a failed, skipped, or unvalidated
candidate. Post-publish failure does not rewrite the tag; it marks the release
verification failed until corrected by a new release.

## Diagnostics

Stable diagnostics use these families:

| Prefix | Boundary |
| --- | --- |
| `ACSDK-LOCK-*` | consumer lock and downloaded artifact identity |
| `ACSDK-PLAN-*` | source/config/SDK validation and plan publication |
| `ACSDK-EMIT-*` | plan verification, generation, locking, and publication |
| `ACSDK-ABI-*` | generated runtime query and exact ABI checks |
| `ACSDK-CMAKE-*` | component selection and transitive runtime dependencies |
| `ACSDK-RELEASE-*` | candidate manifest, checksums, relocation, and publish graph |

A diagnostic states what failed, the observed and expected identities when
applicable, and the action required to correct it. Unsupported constructs remain
explicit verifier failures and are never silently downgraded to another backend.

## Release stop conditions

Do not create the canonical tag until all of these are true:

- Decisions 0232–0235 are implemented and verified.
- Every R01–R20 and R23 item in issue #61 has merged evidence.
- Both exact candidate archives and all wheels pass installed relocation tests.
- The release source revision is final and all manifests use it.
- An independent architecture review has no unresolved blocker.

After publication, complete R21 and R22 by redownloading the release, verifying
hashes, and rerunning the generic installed-model smoke tests.
