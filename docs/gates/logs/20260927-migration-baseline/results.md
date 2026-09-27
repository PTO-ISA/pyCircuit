# Migration baseline results

Source identity: `8887e6dec7b4cc530a9967c860dc6a224d79a4ab`.

The working tree was dirty before baseline execution. Concurrent governance work added or modified `AGENTS.md`, contributor/review workflow documents, `mkdocs.yml`, modernization/governance documents, review/work-item directories, and `examples/davo/`. Product source and tests were not edited by this lane.

## Authoritative native build

- Build: `.pycircuit_out/toolchain/build-llvm22-autodetect`
- Source: `/Users/zhoubot/linx-isa/tools/pyCircuit`
- Generator/configuration: Ninja, Release
- LLVM/MLIR: 22.1.8 from `/opt/homebrew/Cellar/llvm/22.1.8`
- Compiler: Apple clang 21.0.0
- Tests: `PYC_BUILD_AGENTIC_CIRCUIT_TESTS=ON`, `ACIR_BUILD_TESTING=ON`
- Test tools: Python 3.14.6, lit 18.1.8, Verilator 5.044

No active build/test process owned the build directory before reconfiguration. Required tools, the native Python extension, and selected test binaries were rebuilt from the current checkout. Linker warnings only reported duplicate static libraries.

## Results

| Lane | Result | Interpretation |
| --- | --- | --- |
| pyCircuit unit G0 | 217 passed | Earlier governance-language and `.codex` repository-inventory integration failures are resolved without weakening exact-root equality. |
| Agentic contract checker | passed | Earlier governance-document zero-content-identity failure is resolved: `repository contracts: OK (public schemas, 35 stdlib components, LLVM 22.1.8)`. |
| Agentic contract unittest | 29 passed | Earlier propagated governance-document failure is resolved. |
| Agentic CLI | 6 passed | CI-selected `test_all_commands.py` and `test_workspace.py`. |
| Agentic frontend | 426 passed, 1 skipped | Current-source Release build at the first hardcoded tool path; native tools, binding, gfsim, and `acc` are consistent. |
| Focused native C++ | 20 passed | Source units, family metadata/reuse, queue/scalar/register behavior, exact integer widths, link behavior, and post-split rejection. |
| Focused lit | 7 passed | Package/header linking, family child specialization, bit-width lowering, scalar queue/register PYC, C++ syntax compile, bundle headers, and Verilog emission. |
| Executable C++/Verilator baseline | 1 passed | `architecture-obligation-backends.mlir` executes the generated C++ pass/fail models and Verilator pass/fail binaries from one PYC input, including assertion behavior and deterministic emission. |
| Executable family parity candidate | 1 failed before backend | `family-python-multilane-backends.mlir` stops in source-unit compilation with `ACIR-EMIT-002`: a compiled AC unit contains a module body owned by another Python source. No C++ or Verilator execution occurs. |

The initial Agentic frontend run used the pre-existing default AC build and produced 415 passes, 7 skips, and 5 QueueGraph C++ failures. A mixed-build diagnostic rerun selected current integrated `acir-opt-internal`, queue C++ generator, gfsim, and native Python extension, while `test_multi_unit_package.py::_repository_tool()` selected an older hardcoded `acc`; it produced 423 passes, one failure, and three errors. Both hardcoded build roots were then validated as belonging to this checkout with LLVM/MLIR 22.1.8 and no live users. The standalone Debug AC tools were clean-rebuilt. Because test discovery prefers `.pycircuit_out/toolchain/build/bin/acc`, that Release build's `acc`, opt, C++ generator, native extension, and gfsim were also cleaned and rebuilt from current source. The consistent rerun passed 426 tests with one skip. The earlier failures are retained as stale/mixed-build diagnostics and are resolved.

## Focused native inventory selected

- Source-unit/package/link: `CompilerDriverTest.PublishesOneDirectSourceOwnedUnitAndInterface`, `CompilerDriverTest.RejectsWholeDesignPostCompileSplitting`, `CodeGen/acc-package-driver.mlir`.
- Family: `ACIR/static-family-attributes.mlir`, `CodeGen/family-hard-break-absence.mlir`, `CodeGen/queue-family-child-cxx.mlir`, and two `QueueGraphPlanTest` family/reuse cases.
- Scalar/queue/register: `QueueGraphPlanTest.EmitsCanonicalScalarQueuePyc`, `QueueBlocksTest.StatefulTableReadsOldDataAndCommitsWriteAtTickEnd`, queue type round trips, `CodeGen/queue-lanes-pyc.mlir`.
- Integer widths: all eight `UIntTest` cases, `ACIROpsTest.SemanticPrimitiveWidthsAcceptOneTo64AndReject65To130`, 64/65-bit storage boundary, and `CodeGen/bit-widths.mlir`.
- C++/Verilog smoke: `CodeGen/acc-driver.mlir` compiles generated C++ syntax, verifies bundle output, and emits/checks Verilog.
- Executable dual backend: `CodeGen/architecture-obligation-backends.mlir` runs C++ and Verilator pass/fail models. The family/multilane executable candidate is separately retained as a source-unit ownership failure.

## Remaining uncertainty

Required Python G0 passes. The remaining focused gap is the executable finite-family/multilane test, which fails at source-unit ownership before either backend. Release closure, full CTest, full lit, simulations, and nightly gates were intentionally not run.
