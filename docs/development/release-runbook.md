# Local release preview

This page describes a bounded, local preview of the current pyCircuit build
and consumer path. It does not establish release readiness, represent that any
gate has passed, or publish an artifact. Record the exact checkout revision
and command results before drawing a conclusion about a candidate.

M5 remains accepted for its documented scalar profile, and M6-01/M6-02 remain
accepted only for their recorded candidate identities. Candidate-specific M7-01
results are recorded in the [local preview packet](../work-items/m7-local-preview.md);
this page describes reproduction, not a stable-release acceptance claim.

## Configure and build locally

The `release` configure preset builds the CompilerDev and Runtime components
with the existing root CMake options. It uses the local
`.pycircuit_out/toolchain/build` and `.pycircuit_out/toolchain/install`
directories. CompilerDev requires LLVM and MLIR 22.1.8 exactly.

From the repository root, select the matching LLVM CMake package directories
and build the current helper and runtime targets. Set `LLVM_CONFIG` to the
`llvm-config` executable from the LLVM 22.1.8 installation first:

```bash
test "$("${LLVM_CONFIG}" --version)" = "22.1.8"
export LLVM_DIR="$("${LLVM_CONFIG}" --cmakedir)"
export MLIR_DIR="$(dirname "${LLVM_DIR}")/mlir"
export MLIR_OPT="$("${LLVM_CONFIG}" --bindir)/mlir-opt"

cmake --preset release
cmake --build --preset release-tools --parallel 4
cmake --install .pycircuit_out/toolchain/build \
  --prefix "$PWD/.pycircuit_out/toolchain/install"
```

The `release-tools` build preset selects the installed source-unit, design,
and C++ source-parts helpers together with `pyc6_runtime`. The native backend
closure harness is a separate build-tree tool used by the broader semantic
closure. Build the full test tree below when validating that closure.

## Run the bounded preview

Point the existing example gates at this local install and build tree, then run
the repository's current source-owned example and Runtime/RTL smoke scripts:

```bash
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
export PYC_BUILD_DIR="$PWD/.pycircuit_out/toolchain/build"
export PYCIRCUIT_NATIVE_BUILD="$PYC_BUILD_DIR"
export PYCIRCUIT_COMPILER_INSTALL="$PYC_TOOLCHAIN_ROOT"
export PYCIRCUIT_M6_PREFIX="$PYC_TOOLCHAIN_ROOT"
export PATH="${PYC_TOOLCHAIN_ROOT}/bin:${PYC_BUILD_DIR}/bin:${PATH}"

bash flows/scripts/run_examples.sh
bash flows/scripts/run_sims_nightly.sh
python3 -m pytest -q \
  tests/system/test_m6_publication_process_recovery.py \
  tests/system/test_m6_relocated_compiler.py
python3 -m pytest -q \
  tests/system/test_m6_incremental_build.py \
  tests/system/test_m6_example_build.py \
  tests/unit/test_m6_measurement.py \
  tests/unit/test_m6_prepare_output.py
python3 flows/tools/check_m5_retirement.py --install-root "$PYC_TOOLCHAIN_ROOT"
```

Capture the checkout SHA, command lines, exit statuses, and produced evidence
before reporting these preview results. A passing local subset is evidence for
that checkout and those commands only.

## Complete local candidate matrix

Use a Python environment with the project's development/documentation extras.
The M7-01 packet builds a fresh native test tree and runs the current semantic
script, which includes the earlier example/simulation fixture groups and the M6
regressions. This avoids counting the same fixture repeatedly as new coverage.
The two V44 scheduling/reordering cases remain explicitly deselected by that
script; build parallelism does not demonstrate parallel simulation.

```bash
export PYC_BUILD_DIR="$PWD/.pycircuit_out/m7-01-root"
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/m7-01-install"
export PYCIRCUIT_NATIVE_BUILD="$PYC_BUILD_DIR"
export PYCIRCUIT_COMPILER_INSTALL="$PYC_TOOLCHAIN_ROOT"
export PYCIRCUIT_M6_PREFIX="$PYC_TOOLCHAIN_ROOT"
export PYC_GATE_RUN_ID=20261001-m7-preview
export CMAKE_BUILD_PARALLEL_LEVEL=4
PYC_BUILD_TESTING=ON bash flows/scripts/pyc build \
  --build-dir "$PYC_BUILD_DIR" --install-prefix "$PYC_TOOLCHAIN_ROOT"
export PATH="${PYC_TOOLCHAIN_ROOT}/bin:${PYC_BUILD_DIR}/bin:${PATH}"
ctest --test-dir "$PYC_BUILD_DIR" -R ACIR --output-on-failure
bash flows/scripts/run_semantic_regressions_v6.sh
python3 -m pytest tests/unit -m unit -q
python3 -m pytest tests/system/test_m5_model_abi.py -q
python3 flows/tools/check_m5_retirement.py --install-root "$PYC_TOOLCHAIN_ROOT"
pre-commit run --all-files
mkdocs build --strict
```

For a wheel installation smoke, use the existing wheel builder against that
fresh prefix. Explicitly select a wheel tag matching the tested host generation;
M7-01 used `macosx_26_0_arm64` on macOS 26.6.2. That override restricts this
disposable artifact and does not establish the formal SDK's macOS 15 minimum.
Install the existing build dependencies in a disposable packaging environment;
development/documentation extras alone do not provide them.

```bash
python3 -m venv .pycircuit_out/m7-01-packaging-venv
.pycircuit_out/m7-01-packaging-venv/bin/python -m pip install setuptools wheel
.pycircuit_out/m7-01-packaging-venv/bin/python packaging/wheel/create_wheel.py \
  --install-dir "$PYC_TOOLCHAIN_ROOT" \
  --out-dir .pycircuit_out/m7-01-local-wheel \
  --platform macos-arm64 --wheel-plat-name macosx_26_0_arm64
export PYCIRCUIT_M5_WHEEL="$PWD/.pycircuit_out/m7-01-local-wheel/pycircuit_hisi-6.1.0-py3-none-macosx_26_0_arm64.whl"
python3 -m pytest tests/system/test_m5_wheel_install.py -q
```

Use the candidate's actual version and a fresh output directory when reproducing
on another revision. Archive command results and wheel/source bindings as gate
evidence. No SDK release index or public attestation is created by this smoke.

## Stable release boundary

Stable release validation and publication remain owned by
`.github/workflows/release.yml`. That workflow checks an
exact source commit and version, runs the full repository closure, builds and
verifies Linux x86_64, macOS arm64, and Windows x86_64 candidates, binds
platform manifests and attestations to the commit, and checks the published
bytes and stable platform downloads. The local preview above does not perform
those multi-platform or publication checks and cannot replace them.
