# Reproduction commands

All commands ran in `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Native helper overrides point to this checkout's `.pycircuit_out/w10-pm/build/bin`.
The standalone clean compiler build uses `.pycircuit_out/m4-clean-native`.

```sh
cmake -S compiler/acir -B .pycircuit_out/m4-clean-native -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/opt/homebrew/opt/llvm@22/lib/cmake/llvm \
  -DMLIR_DIR=/opt/homebrew/opt/llvm@22/lib/cmake/mlir
cmake --build .pycircuit_out/m4-clean-native --parallel 4 --target \
  acir-source-unit-harness acir-design-harness acir-cpp-source-parts-harness
cmake -S tests/integration/agentic-circuit/m4-preview \
  -B .pycircuit_out/m4-documented -G Ninja \
  -DPYCIRCUIT_NATIVE_BUILD="$PWD/.pycircuit_out/m4-clean-native"
cmake --build .pycircuit_out/m4-documented --target preview-build --parallel 4
```

The exact three documented runner/oracle argv arrays and exit codes are in
`documented-run.json`. Both event captures are stored beside this file.
All above commands returned 0; initial native build completed 106 steps.

```sh
PYCIRCUIT_NATIVE_BUILD="$PWD/.pycircuit_out/w10-pm/build" \
  uv run --no-project --offline --with pytest pytest -q \
  tests/system/test_m4_preview_workflow.py tests/unit/test_m4_runner_protocol.py \
  --junitxml=.pycircuit_out/m4-completion-tests/final.xml
```

That independent-author invocation used Python 3.12.12 / pytest 9.1.1 and returned
0, with 27 passing cases and no skips. The PM also repeated the ten oracle cases
with Python 3.14.6 / pytest 9.0.2; see oracle-unit.xml.

The existing lane used `/opt/homebrew/Cellar/pytest/9.0.2_1/libexec/bin/python -m pytest -q`
with the following files and explicit ACIR_SOURCE_UNIT_HARNESS,
ACIR_DESIGN_HARNESS and ACIR_CPP_SOURCE_PARTS_HARNESS overrides:

```text
tests/system/test_final_scalar_declarations.py
tests/system/test_cpp_source_parts.py
tests/system/test_generic_multi_assignment.py
tests/system/test_generic_assignment_roundtrip.py
tests/system/test_source_design_bridge.py
tests/system/test_masked_next_register.py
tests/system/test_source_unit_cmake_build.py
tests/unit/test_generated_bundle.py
tests/unit/test_publication.py
tests/unit/test_publication_fs.py
tests/unit/test_source_unit_files.py
tests/unit/test_driver_commands.py
```

It returned 0, with 331 passed / 3 Windows-only skips. Native targets were rebuilt
from this checkout, then CTest ran exactly these seven binaries, all passing:

```sh
ctest --test-dir .pycircuit_out/w10-pm/build --output-on-failure \
  -R '^(ACIRFinalProgramTests|ACIRExecutableBackendClosureTests|ACIRObservationTests|ACIRObservationContractsTests|ACIRCheckContractsTests|ACIRSystemLifecycleTests|ACIRSimExecutorTests)$' \
  --output-junit "$PWD/.pycircuit_out/m4-completion-probe/native-regression.xml"
```

The original invocation supplied a relative JUnit path, which CTest resolved
under its build directory. The raw XML was copied unchanged to this archive;
the command above uses an absolute destination to avoid that path ambiguity.

```sh
mkdocs build --strict -d .pycircuit_out/m4-docs
pre-commit run --files <the candidate changed files>
```

Both returned 0. Native linker duplicate-library warnings are pre-existing;
Windows execution, installed SDK, and full release gates are not claimed.
