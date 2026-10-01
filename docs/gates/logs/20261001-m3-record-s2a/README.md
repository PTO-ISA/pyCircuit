# M3-E01 S2A acceptance evidence

Date: 2026-10-01. Checkout:
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Base: `4c3a4be690f30453acdc551e224e9ef258291ee1`.
Approved C2-DECL-R B: `ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`.

19 code/test/ODS/CMake files: [candidate.json](candidate.json), aggregate
`eaee80593852c78d3a5c5150c42afe47184283d669b72fb5599964914040e3ec`.
15 compiler/ODS/CMake files: [source.json](source.json), aggregate
`95e3469d0cb433ce0b510c263b179d4e860da0227837de22f34fdd68d5032238`.
Both use SHA-256 of sorted path + NUL + file SHA-256 hex + LF entries.
Documentation and raw evidence are outside the code/test aggregate.

## Scope and independent acceptance

This is the strict intermediate rule/op packet: one owned record current/target,
read/get(0)/get(1)/create, one next use and the exact two-leaf selector.
Nominal authority remains in a separate provider implementation/declaration unit.
The shared verifier checks actual handles, block arguments, domains, ValueIDs,
ordered SSA, origins, control SSA and complete operation/use inventory.
AnyType widening is immediately narrowed in common verification. Incorrect
`ac.required_records` placement and scalar/numeric downgrade routes reject.

Empty local expansion, true entry paths and constant next-use paths are the
current seam. Python producer/helpers, dynamic paths, arithmetic/check/observe
composition, S2C materialization and S3/S4 hardware execution remain closed.
Whole hardware and emit must reject this packet. A parser with auto-verify
disabled only permits explicit rule-level verification; it is not product admission.

- [Independent Sol APPROVE](review.md), review SHA-256
  `eb3b020aef332fa655bcac08cd539961977bb7cb513d06d0bfc19005234ff73a`.
- [Independent Astra CONFORMANT](architecture-conformance.md), bound to the source
  aggregate; the older associated test manifest in that note is not execution evidence.
- Two separate Luna implementation lanes, independent Luna tests, Sol fixture
  debugger for SSA lifetime, and PM integration. No semantic assertion was loosened.

## Raw results

| Lane | Result | Evidence |
| --- | --- | --- |
| New record native | 11 pass | [record.xml](record.xml) |
| Reviewer record rerun | 11 pass | [reviewer-record.xml](reviewer-record.xml) |
| Reviewer full FinalProgram | 69 pass | [reviewer-final-program.xml](reviewer-final-program.xml) |
| Native excluding known Math binary | 19 binaries / 279 cases, zero fail/error/skip/disabled | [native.xml](native.xml), native-details XML |
| S1/scalar/source/namespace/public compile-link | 129 pass, zero fail/error/skip | [regressions.xml](regressions.xml) |
| SourceMath | 88 pass / 3 known baseline failures out of 91 | [math.xml](math.xml) |
| Same tests with pristine baseline, isolated processes | 0 pass / 2 ordinary test failures / 9 SIGABRT | [report](failing-first-report.json), failing-first-cases logs |

Baseline source is a fresh `git archive 4c3a4be6...` under
`.pycircuit_out/m3-s2a-baseline-source`; baseline compiler and harnesses were built
there, with identical LLVM/MLIR 22.1.8, unset build type, compiler/runtime/testing
options and four jobs. Only the exact final three test files and their CMake
registration were overlaid, byte-identical to candidate tests. No native artifact
was copied from candidate or another worktree. The old typed ValueBinding getter
casts a record to IntegerType and aborts in the old generic verifier; the
[LLDB stack](baseline-debug.log) shows this precise missing capability. Do not
summarize signal terminations as eleven ordinary failures or as no baseline errors.

The three SourceMath/APInt failures are the same previously reproduced baseline
cases documented in S1. No full 20-binary PASS is claimed and no skip hides them.
Earlier fixture syntax/API/lifetime failures are not product failing-first evidence.

## Reproduction

```sh
cmake -S . -B .pycircuit_out/m3-s2a-build -G Ninja \
  -DPYC_BUILD_COMPILER_DEV=ON -DPYC_BUILD_TESTING=ON -DPYC_BUILD_RUNTIME_LIB=ON \
  -DLLVM_DIR=/opt/homebrew/opt/llvm@22/lib/cmake/llvm \
  -DMLIR_DIR=/opt/homebrew/opt/llvm@22/lib/cmake/mlir \
  -DCMAKE_INSTALL_PREFIX="$PWD/.pycircuit_out/m3-s2a-install"
cmake --build .pycircuit_out/m3-s2a-build -j 4
cmake --install .pycircuit_out/m3-s2a-build
.pycircuit_out/m3-s2a-build/bin/ACIRFinalProgramTests \
  --gtest_filter='FinalRecordValue*'
GTEST_OUTPUT="xml:$PWD/.pycircuit_out/m3-s2a-native-details/" \
  ctest --test-dir .pycircuit_out/m3-s2a-build --no-tests=error --output-on-failure \
  -R '^ACIR' -E '^ACIRSourceMathContractsTests$'
export ACIR_SOURCE_UNIT_HARNESS="$PWD/.pycircuit_out/m3-s2a-build/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PWD/.pycircuit_out/m3-s2a-build/bin/acir-design-harness"
export ACIR_CPP_SOURCE_PARTS_HARNESS="$PWD/.pycircuit_out/m3-s2a-build/bin/acir-cpp-source-parts-harness"
.venv/bin/python -m pytest tests/system/test_final_record_declarations.py \
  tests/system/test_final_scalar_declarations.py tests/system/test_source_unit_packet.py \
  tests/system/test_source_namespace_bindings.py tests/system/test_driver_compile_link.py -q
```

Source annotations use supported upper-only `range(upper)`. The native fixture
then edits final field lower bounds to one for the inactive-zero oracle; it does
not claim Python supports lower/upper range annotations. Record reset and both
backends remain unimplemented. Strict docs/hooks are recorded separately.
