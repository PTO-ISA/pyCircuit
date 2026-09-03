# pyCircuit 6 agent instructions

This repository follows the pyCircuit 6 frontend contract. CycleAwareSignal is
the primary authoring model, and the V6 documents are the current product source
of truth.

## Read first

- `docs/v6_PyCircuit_Specification.md`
- `docs/rfcs/pyc6-decisions.md`
- `docs/pyc6-plan.md`
- `docs/development/contributing-workflow.md`
- `docs/development/testing-and-gates.md`
- `docs/development/review-and-merge.md`

## Codex skills

- Apply `$pyc6` first for hard contracts and evidence expectations.
- Use `$pyc-build-v60` when running builds or gate lanes.
- Use `$linx-pycircuit` when touching Linx integration flows.

## Local build environment

This section is host-specific. On this `aarch64-unknown-linux-gnu` machine,
select the environment below before configuring, building, or testing. Do not
first try `/usr/bin/python3`, the host GCC 10, or default CMake preset paths.

```bash
export PYC_LOCAL_ROOT=/home/lc/pyCircuit
export LLVM_PREFIX=/home/lc/opt/llvm-22.1.8
export ACIR_LOCAL_TOOLCHAIN=/home/lc/opt/agentic-circuit-toolchain
export ACIR_LOCAL_GCC=/home/lc/opt/gcc14
export PYC_LOCAL_BUILD_DIR="$PYC_LOCAL_ROOT/.pycircuit_out/local-gcc14-llvm22/build"
export PYC_LOCAL_INSTALL_DIR="$PYC_LOCAL_ROOT/.pycircuit_out/local-gcc14-llvm22/install"
export PYC_LOCAL_PYTHON="$ACIR_LOCAL_TOOLCHAIN/python-env/bin/python"
export CC="$ACIR_LOCAL_GCC/bin/aarch64-conda-linux-gnu-gcc"
export CXX="$ACIR_LOCAL_GCC/bin/c++"
export PATH="$ACIR_LOCAL_GCC/bin:$LLVM_PREFIX/bin:$ACIR_LOCAL_TOOLCHAIN/bin:$ACIR_LOCAL_TOOLCHAIN/python-env/bin:$PATH"
export LD_LIBRARY_PATH="$ACIR_LOCAL_GCC/lib:$LLVM_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PYTHONPATH="$PYC_LOCAL_ROOT:$PYC_LOCAL_ROOT/components/agentic-circuit/src:$PYC_LOCAL_BUILD_DIR/components/agentic-circuit/python${PYTHONPATH:+:$PYTHONPATH}"
```

Verified versions and paths (2026-09-01):

- Python 3.11.16: `/home/lc/opt/agentic-circuit-toolchain/python-env/bin/python`
- GCC/G++ 14.4.0: `/home/lc/opt/gcc14/bin`
- LLVM/MLIR 22.1.8: `/home/lc/opt/llvm-22.1.8`
- CMake 3.31.10 and Ninja 1.13: `/home/lc/opt/agentic-circuit-toolchain/bin`
- Python lit entry point: `/home/lc/opt/agentic-circuit-toolchain/python-env/bin/lit`
- GTest: `/home/lc/opt/agentic-circuit-toolchain/gtest/lib64/cmake/GTest`
- zstd compatibility library: `/home/lc/opt/agentic-circuit-toolchain/cmake-deps/zstd-compat`
- libxml2 headers: `/home/lc/opt/agentic-circuit-toolchain/cmake-deps/libxml2-include`
- libxml2 library: `/usr/lib64/libxml2.so.2`

Configure, build, and install the whole project with explicit local
dependencies. Before building, inspect the effective CPU count, available
memory, cgroup quotas, and current host load, then set
`PYC_LOCAL_BUILD_JOBS` to a reasonable positive integer for that run. Do not
use a fixed repository-wide job limit. Use the available capacity on a
dedicated server; on a shared host, leave enough headroom for other workloads
and reduce the value if the build encounters memory or I/O pressure. Honor an
explicit value supplied by the user or environment.

```bash
"$ACIR_LOCAL_TOOLCHAIN/bin/cmake" \
  -S "$PYC_LOCAL_ROOT" -B "$PYC_LOCAL_BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX="$PYC_LOCAL_INSTALL_DIR" \
  -DCMAKE_C_COMPILER="$CC" \
  -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_MAKE_PROGRAM="$ACIR_LOCAL_TOOLCHAIN/bin/ninja" \
  -DLLVM_DIR="$LLVM_PREFIX/lib/cmake/llvm" \
  -DMLIR_DIR="$LLVM_PREFIX/lib/cmake/mlir" \
  -DGTest_DIR="$ACIR_LOCAL_TOOLCHAIN/gtest/lib64/cmake/GTest" \
  -Dzstd_INCLUDE_DIR="$ACIR_LOCAL_TOOLCHAIN/cmake-deps/zstd-compat/include" \
  -Dzstd_LIBRARY="$ACIR_LOCAL_TOOLCHAIN/cmake-deps/zstd-compat/lib/libzstd.a" \
  -Dzstd_STATIC_LIBRARY="$ACIR_LOCAL_TOOLCHAIN/cmake-deps/zstd-compat/lib/libzstd.a" \
  -DLIBXML2_INCLUDE_DIR="$ACIR_LOCAL_TOOLCHAIN/cmake-deps/libxml2-include" \
  -DLIBXML2_LIBRARY=/usr/lib64/libxml2.so.2 \
  -DPython3_EXECUTABLE="$PYC_LOCAL_PYTHON" \
  -DACIR_LIT_EXECUTABLE="$ACIR_LOCAL_TOOLCHAIN/python-env/bin/lit" \
  -DACIR_ENABLE_ASSERTIONS=ON \
  -DPYC_BUILD_MLIR_TOOLS=ON \
  -DPYC_BUILD_RUNTIME_LIB=ON \
  -DPYC_BUILD_AGENTIC_CIRCUIT=ON \
  -DPYC_BUILD_AGENTIC_CIRCUIT_TESTS=ON
"$ACIR_LOCAL_TOOLCHAIN/bin/cmake" --build "$PYC_LOCAL_BUILD_DIR" \
  --parallel "$PYC_LOCAL_BUILD_JOBS"
"$ACIR_LOCAL_TOOLCHAIN/bin/cmake" --build "$PYC_LOCAL_BUILD_DIR" \
  --target pyc-opt --parallel "$PYC_LOCAL_BUILD_JOBS"
"$ACIR_LOCAL_TOOLCHAIN/bin/cmake" --install "$PYC_LOCAL_BUILD_DIR"
```

`pyc-opt` is intentionally excluded from the default `all` target but is
currently required by the install rules, so build it explicitly before
installing.

For Agentic Circuit smoke validation, use the generated Python resource tree
or the isolated install; an editable source tree alone does not provide the
generated schemas. For example:

```bash
export PYTHONPATH="$PYC_LOCAL_INSTALL_DIR/lib/python3.11/site-packages"
export PATH="$PYC_LOCAL_INSTALL_DIR/bin:$PATH"
export LD_LIBRARY_PATH="$PYC_LOCAL_INSTALL_DIR/lib:$PYC_LOCAL_INSTALL_DIR/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
"$PYC_LOCAL_PYTHON" -m agentic_circuit._cli doctor --json
```

Full-suite prerequisites currently absent on this host:

- `$LLVM_PREFIX/bin/FileCheck`, `split-file`, `not`, and `count`
- Verilator

Do not report `check-acir` or Verilog lanes as validated until those tools are
installed. The outer `$ACIR_LOCAL_TOOLCHAIN/bin/lit` is a shell wrapper; CMake
invokes lit through Python, so `ACIR_LIT_EXECUTABLE` must use the Python entry
point documented above.

## Clang-format gate

Before every commit or push, run the owned-source check with the pinned LLVM
22 formatter rather than an ambient `clang-format`:

```bash
git ls-files '*.cpp' '*.h' ':!references/**' |
  xargs /home/lc/opt/llvm-22.1.8/bin/clang-format --dry-run --Werror
```

## Agentic Circuit staged validation

Most Agentic Circuit development should use staged validation so the edit-test
loop does not repeatedly pay for full pyCircuit closure.

### During development: narrow Agentic Circuit checks

- Start with the smallest tests that cover the files and semantics being
  changed. Do not run whole-repository pyCircuit gates after every edit.
- For ACPy/frontend work, run the affected tests under
  `components/agentic-circuit/tests/python_frontend/` and any directly related
  contract or end-to-end test.
- For ACIR, ACSim, QueueGraph, codegen, or gfsim work, build only the required
  targets such as `acir-opt`, `ACIROpsTests`, `GfsimTests`, or `CodeGenTests`,
  then select the corresponding CTest cases with `ctest -R`.
- Use `PYC_LOCAL_BUILD_JOBS` for both build and CTest parallelism. Keep the
  existing build directory warm and avoid reconfiguring or reinstalling unless
  the changed contract requires it.

Example focused commands:

```bash
"$PYC_LOCAL_PYTHON" -m unittest \
  components/agentic-circuit/tests/python_frontend/test_queue_frontend.py
"$PYC_LOCAL_PYTHON" -m unittest \
  components/agentic-circuit/tests/e2e/test_table_backend.py

"$ACIR_LOCAL_TOOLCHAIN/bin/cmake" --build "$PYC_LOCAL_BUILD_DIR" \
  --target acir-opt ACIROpsTests GfsimTests CodeGenTests \
  --parallel "$PYC_LOCAL_BUILD_JOBS"
"$ACIR_LOCAL_TOOLCHAIN/bin/ctest" --test-dir "$PYC_LOCAL_BUILD_DIR" \
  -R '^(ACIROpsTests|GfsimTests|CodeGenTests)$' \
  --output-on-failure --parallel "$PYC_LOCAL_BUILD_JOBS"
```

### Feature checkpoint: Agentic Circuit G0/G1

- When an Agentic Circuit feature is functionally complete, run the applicable
  AC G0 frontend, schema, contract, and inventory tests.
- Run AC G1 when the change touches ACIR/ACSim, verifiers, transformations,
  QueueGraph, native code generation, or gfsim. This includes the full Agentic
  Circuit C++ unit set, `check-acir` when its required tools are available, and
  at least one relevant end-to-end case.
- Run changed-file pre-commit checks at this checkpoint. If a required local
  tool is unavailable, record that gap instead of silently substituting an
  unrelated whole-repository lane.

### Before push or PR: pyCircuit repository closure

- After the Agentic Circuit feature and its G0/G1 checks are stable, run the
  pyCircuit whole-repository gates required by
  `docs/development/testing-and-gates.md`, including changed-file or all-file
  pre-commit, examples, simulations, semantic regressions, documentation, and
  strict decision-status checks as applicable.
- Add the nightly simulation lane for changes to examples, testbenches,
  simulation orchestration, Verilog behavior, or other risks mapped to that
  lane by the testing matrix.
- Do not defer integration validation until the end when a change directly
  touches ACIR-to-PYC, `libpyc6_runtime`, synthesizable semantics, shared
  pyCircuit APIs, or Linx interfaces. Run the affected integration lane as soon
  as that boundary changes, then repeat the required closure before submission.

## Task mapping

- Issue fix or feature work: identify affected decision IDs, then map the change
  to the required gates in `docs/development/testing-and-gates.md`.
- Code review: prioritize semantic regressions, missing gate coverage,
  incorrect evidence paths, and documentation drift before style issues.
- PR preparation: include decision IDs, gate commands, evidence paths, doc
  updates, and compatibility or risk notes.
- Documentation updates: keep the V6 specification, contributor docs, README,
  and actual repository workflow aligned.

## Hard rules

- Keep CycleAwareSignal, CycleAwareDomain, and automatic cycle balancing as
  first-class pyCircuit 6 design contracts (Decision 0148).
- Add or tighten MLIR verifiers or passes before changing semantics.
- Do not implement semantic fixes in only one backend. Semantics live in the
  dialect, passes, and verifiers.
- Build and test from the current checkout. Never copy staged toolchains,
  shared libraries, or generated artifacts from another worktree.
- Do not place temporary tests, scripts, examples, or design notes in the repo
  root. Use the existing test, example, documentation, or disposable output
  directories.
- Treat public examples as product surface. New examples must provide
  user-facing design coverage, compile-flow coverage, or semantic evidence.
- Reference affected decision IDs and attach semantic or decision-bearing gate
  evidence under `docs/gates/logs/<run-id>/`.
- Generated gate JSON, especially `decision_status_report.json`, must end with
  one LF newline. After generating evidence, run the applicable pre-commit
  checks on the exact files before committing or pushing; do not assume the
  report generator supplies the final newline.
- Keep the repository hard-break only. Do not restore removed compatibility
  modes or label the current CycleAwareSignal API with a prior product version.
- Keep active runtime, trace, and semantic-gate names on the pyCircuit 6
  contract: `libpyc6_runtime`, `PYC6TRC3`, and
  `run_semantic_regressions_v6.sh`.
- Do not add AI co-author lines to commits or pull request text.

## Repository authority

- `PTO-ISA/pyCircuit` is the upstream source of truth and release authority.
- `LinxISA/pyCircuit` is a downstream fork for Linx integration.
- Product decisions, general fixes, and reusable Linx changes should land
  upstream first whenever practical.
- See `docs/development/repository-management.md` for branch, release, and fork
  synchronization policy.

## When to stop and ask

- The requested change conflicts with an accepted pyc6 decision.
- The work would change documented semantics without a clear decision update.
- Unrelated user changes overlap the same files and the merge strategy is
  ambiguous.
- Required credentials or external tooling block required validation or
  publishing.

## Working expectations

- Start with the smallest reproducer and narrowest gate lane that proves the
  change; widen only as required by risk.
- Keep generated logs bounded and archive only reviewable evidence.
- Update behavior documentation in the same change as the behavior.
- Report non-critical local validation gaps explicitly instead of hiding them.
