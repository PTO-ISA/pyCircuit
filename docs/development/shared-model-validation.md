# Shared-model tool validation

This tool-only change builds and runs without any CPU demo directory. Its tested
implementation is2d05482f2c1bde4354de260cdea2181ac57e139a against main24fa898;
the system project test was renamed to avoid a same-basename unit-test collection
collision, with its nightly selector updated and all test assertions retained.
Publication adds this reviewable summary and formatting/documentation updates;
no source computation or numerical expectation is changed by those edits.

The accompanying `shared-model-validation.json` records machine-readable scope.
Local raw logs remain ignored; this public summary does not pretend a local path
is a downloadable CI artifact. Remote CI outcomes belong to the exact PR head.

| Executed check | Actual outcome |
| --- | --- |
| Fresh standard build/install |Release CompilerDev+Runtime, LLVM/MLIR22.1.8, configure/build/install rc0 |
| `bash tools/run_api_tests.sh --tier gate` |25/25 CTests, rc0 |
| `bash tools/run_examples.sh --tier gate` |13/13 CTests, pinned Verilator5.048, rc0 |
| `pytest tests/system/test_shared_execution_models.py tests/system/test_installed_model_project.py tests/system/test_clocked_fixedpoint.py tests/unit` |847 passed,4 skipped, rc0 |

The generated WordII1/WordII2/Digest and installed project tests use independent
raw/flag/state and finite FIFO timing expectations. Native workers1/2 and RTL
are selected by their existing tests. The complete-state profile additionally
uses a non-CPU hierarchy/provider with actual Icarus execution, exact four-word
results, hidden-state/partial-witness rejection, native/RTL unknown boundaries,
incremental source-only helper reuse, stale-success protection and a timer that
prevents idle folding. These are separate from ISA-specific consumer validation.

Reproduce using the installation and testing guides, then the commands above.
`PYC_BUILD_DIR` and `PYC_TOOLCHAIN_ROOT` must select the build/install from the
same checkout. The source dependency closure and installed project do not rely
on a private bundle or another worktree's compiler. Runtime-only consumers keep
their existing LLVM-free CMake boundary.

Four skips cover Windows filesystem APIs on Linux and optional setuptools
package-discovery coverage. Full nightly and competitive performance measurement
are NOT_RUN. Independent review remains NOT_RUN after the existing configured
reviewer failed HTTP401 before producing an opinion; this is neither review
approval nor rejection. SOTA is NOT_ESTABLISHED.
