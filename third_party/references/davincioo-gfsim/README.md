# DavinciOO gfsim Reference Model

This directory preserves the DavinciOO out-of-order NPU C++ model as a fixed
reference for future ACIR-to-gfsim code generation. It is deliberately outside
the installed Agentic Circuit runtime and public schema catalog.

## Snapshot layout

| Path | Purpose |
| --- | --- |
| [`upstream/`](upstream/) | Selected upstream `model/` files, kept byte-for-byte unchanged |
| [`SOURCE.json`](SOURCE.json) | Repository, commit, subtree, selection, and license status |
| [`UPSTREAM_FILES.sha256`](UPSTREAM_FILES.sha256) | Exact imported-file inventory and SHA-256 lock |
| [`GENERATION_CONTRACT.md`](GENERATION_CONTRACT.md) | Boundary between IR-generated topology and reusable model policies |
| [`CMakeLists.txt`](CMakeLists.txt) | Non-installed wrapper that builds the snapshot without target-name collision |

The selected snapshot includes one 15-record upstream softmax trace. Its smoke
test exercises decode, rename, issue, all required engines and ROB retirement,
and deterministically completes in 453 model cycles.

The snapshot comes from DavinciOO commit
`a542b9cf705096288c615575be222b974b570a18`. Do not edit files under
`upstream/` in place. Refresh the snapshot as one reviewed import and update
both provenance files together.

## Build

The normal Agentic Circuit development build includes reference models because
`BUILD_TESTING` is enabled. It can be disabled explicitly:

```sh
cmake --preset dev-llvm22 -DACIR_BUILD_REFERENCE_MODELS=OFF
```

The reference also builds independently and has no LLVM, MLIR, Python, or PTO
runtime dependency:

```sh
cmake -S references/davincioo-gfsim \
  -B build/davincioo-gfsim-reference -G Ninja
cmake --build build/davincioo-gfsim-reference
build/davincioo-gfsim-reference/davincioo-gfsim-reference --help
```

The wrapper changes only the CMake target name. All selected C++, topology
documentation and the smoke trace remain byte-identical to the locked source
snapshot. The upstream model README is intentionally replaced by this snapshot
README because it documents scripts, tests and toolchain submodules that are
outside the reference boundary.

Because `upstream/` is a verbatim external snapshot, repository formatting and
static-analysis gates exclude it. The wrapper, provenance, tests and generation
contract remain normal owned Agentic Circuit sources and are covered by the
repository gates. A path-specific Git whitespace attribute preserves one
upstream blank-line trailing-space occurrence without weakening checks for
owned files.

## Licensing boundary

The source DavinciOO repository currently contains a repository-license
placeholder stating that it does not publish a final blanket license grant.
Accordingly, this snapshot is recorded as `unresolved` in `SOURCE.json` and is
not represented as BSD-3-Clause code, installed, packaged, or linked into public
Agentic Circuit libraries. Resolve upstream licensing before distributing the
snapshot as part of a release artifact.
