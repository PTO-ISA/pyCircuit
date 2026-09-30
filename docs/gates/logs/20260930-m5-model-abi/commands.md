# Commands and environment

Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Base: `26e096c0`; current-checkout native helper directory:
`.pycircuit_out/w10-pm/build/bin`. All commands below returned 0.

```sh
cmake --build .pycircuit_out/w10-pm/build --parallel 4 --target \
  acir-cpp-source-parts-harness acir-design-harness
```

Final combined test command used the following environment:

```sh
export PYCIRCUIT_NATIVE_BUILD="$PWD/.pycircuit_out/w10-pm/build"
export ACIR_SOURCE_UNIT_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-design-harness"
export ACIR_CPP_SOURCE_PARTS_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-cpp-source-parts-harness"
/opt/homebrew/Cellar/pytest/9.0.2_1/libexec/bin/python -m pytest -q \
  tests/system/test_m5_model_abi.py \
  tests/system/test_m4_preview_workflow.py \
  tests/unit/test_m4_runner_protocol.py \
  tests/system/test_final_scalar_declarations.py \
  tests/unit/test_generated_bundle.py \
  --junitxml=.pycircuit_out/m5-abi/final.xml
```

Result: 141 passed, no failures/errors/skips. Python 3.14.6 / pytest 9.0.2,
macOS arm64, LLVM/MLIR 22.1.8 and C++20. Independent ABI-only execution is
archived in independent-abi.xml (12 passed). Tests invoke actual public source
producers/private native emission/CMake and C11 compilation; the C consumer's
include paths contain generated files and Runtime headers, not LLVM/MLIR headers.

The two host-fault cases alter exact-one-match locations in copies of generated
headers, not production source: constructor throw and failed FinalizeBuild.
Each isolated subprocess observes RUNTIME_FAILURE and a cleared output handle.
No Windows, installed-package or complete M5 cutover claim is made.
