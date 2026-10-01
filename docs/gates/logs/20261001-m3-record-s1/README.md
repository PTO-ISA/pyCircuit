# M3-E01 S1 acceptance evidence

Date: 2026-10-01. Checkout:
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Baseline: `1efec35eaa19eab2315797340904def88b24a640`.
Approved C2-DECL-R B: `ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`.

Ten code/test/CMake files are frozen in [candidate.json](candidate.json).
Aggregate: `3405b937fb6ddce11a21214c6ec3722a73a15455087a33101705ea72a55e1159`.
Algorithm: SHA-256 of sorted relative path + NUL + file SHA-256 hex + LF entries.
Documentation and this evidence are separate from the code/test binding.

## Acceptance scope

S1 projects complete owner-defined two-field flat finite record declarations into
the common final package. Constructors become canonical provenance after source
authority and direct-field/valid-path erasure checks; no final function remains.
Unused/private/implementation-owned declarations and empty/facade units survive.
Fresh final verification is source-independent; frozen-object mutation rejects.
Record state/value computation and every backend emission of record-bearing final
remain closed. No executable record, S2–S4, profile or full-E01 acceptance is claimed.

- [Independent Sol APPROVE](reviewer-review.md), SHA-256
  `15682980ba6e61da9d926dc27fe4c7a989628b96a53bb6ca10ca1cfe04c624b8`.
- [Independent Astra CONFORMANT](architecture-conformance.md), seven source files
  bound by [source-freeze.json](source-freeze.json).
- Separate Luna implementation and test instances; PM integrates builds and docs.

## Raw outcomes

| Lane | Outcome | Raw evidence |
| --- | --- | --- |
| Final independent system | 10 pass, no fail/error/skip | [record-system.xml](record-system.xml) |
| Final record native | 4 pass, no fail/error/skip/disabled | [record-native.xml](record-native.xml) |
| Native closure excluding confirmed baseline Math binary | 19 binaries / 268 cases pass, no fail/error/skip/disabled | [native-ctest.xml](native-ctest.xml), [details](native-details/) |
| Installed scalar/source/namespace/public compile-link regressions | 119 pass, no fail/error/skip | [regressions.xml](regressions.xml) |
| Installed public CLI smoke | 12 commands, expected outcomes | [installed-smoke.json](installed-smoke.json) |
| Same final test file on pristine baseline tools | 6 fail / 4 pass, no errors/skips | [failing-first.xml](failing-first.xml), [log](failing-first.log) |
| Reviewer reruns | system 10 / record native 4 / full FinalProgram 58 pass | reviewer XML files |
| Additional provenance probes | 5 specific verifier rejections | [provenance-probes.json](provenance-probes.json) |

The installed smoke emits valid scalar CPP/RTL bundles, compiles a parent with
the provider Python and body absent, links the complete restored closure, removes
source/unit artifacts and freshly verifies saved `design_top.ac`. Both record
emit targets reject without new bundles and preserve previously published scalar
bundle bytes on replace. It executes the installed CLI and installed native tools.

The current-file failing-first failures are four unavailable positive S1 final
paths and two newly specific constructor-admission diagnostics. The four existing
rejection cases pass. The earlier six-case baseline is historical only.
Coherent constructor probes modify every owning/import-snapshot carrier together;
rejection comes from constructor preflight, not a snapshot mismatch. Frozen
location changes reject, while intrinsic standalone `loc(unknown)` remains valid.

## Build and reproduction

Tool versions are in [tool-versions.json](tool-versions.json). Candidate is built
from this checkout using LLVM/MLIR 22.1.8 and four jobs; nothing is copied from
another worktree. Build type is unset, so project assertions remain enabled.

```sh
cmake -S . -B .pycircuit_out/m3-s1-build -G Ninja \
  -DPYC_BUILD_COMPILER_DEV=ON -DPYC_BUILD_TESTING=ON -DPYC_BUILD_RUNTIME_LIB=ON \
  -DLLVM_DIR=/opt/homebrew/opt/llvm@22/lib/cmake/llvm \
  -DMLIR_DIR=/opt/homebrew/opt/llvm@22/lib/cmake/mlir \
  -DCMAKE_INSTALL_PREFIX="$PWD/.pycircuit_out/m3-s1-install"
cmake --build .pycircuit_out/m3-s1-build -j 4
cmake --install .pycircuit_out/m3-s1-build
export ACIR_SOURCE_UNIT_HARNESS="$PWD/.pycircuit_out/m3-s1-install/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PWD/.pycircuit_out/m3-s1-install/bin/acir-design-harness"
export ACIR_CPP_SOURCE_PARTS_HARNESS="$PWD/.pycircuit_out/m3-s1-install/bin/acir-cpp-source-parts-harness"
.venv/bin/python -m pytest tests/system/test_final_record_declarations.py -q
.venv/bin/python -m pytest tests/system/test_final_scalar_declarations.py \
  tests/system/test_source_unit_packet.py tests/system/test_source_namespace_bindings.py \
  tests/system/test_driver_compile_link.py -q
GTEST_OUTPUT="xml:$PWD/.pycircuit_out/m3-s1-native-details/" \
  ctest --test-dir .pycircuit_out/m3-s1-build --no-tests=error --output-on-failure \
  -R '^ACIR' -E '^ACIRSourceMathContractsTests$' \
  --output-junit "$PWD/.pycircuit_out/m3-s1-native-final.xml"
```

Actual independent system/record runs used the corresponding fresh build `bin/`
tools; the regression and installed smoke used the prefix. The recorded CTest
command used a relative JUnit destination, created inside the build directory;
its unchanged bytes were copied here. [Build](build.log), [install](install.log),
hooks and raw XML capture actual outcomes. No native tool was changed afterwards.

## Pre-existing numerical validation gap

The whole 20-binary CTest lane is **not PASS**. `ACIRSourceMathContractsTests`
has three NumericComposition cases whose forked verifier aborts at LLVM APInt
`bitPosition < getBitWidth()` in both candidate and pristine baseline:

- `NumericCompositionAdversarialTest.RejectsCoherentRecipeSsaAndProofCorruption`
- `NumericCompositionAdversarialTest.RejectsConstantRightAndBoundaryAuthorityBypass`
- `NumericCompositionContractsTest.LoweredClosureRejectsInputProofAndArithmeticMutation`

Pristine source was materialized with `git archive 1efec35e...` into
`.pycircuit_out/m3-s1-baseline-source`; baseline libraries/harnesses were freshly
built in `.pycircuit_out/m3-s1-baseline-extra-build`, never copied from candidate.
[Matching flags](baseline-flags.json) include compiler, build type, C++ flags,
architecture, LLVM/MLIR and compiler/runtime/testing options. Baseline full Math
run is 91 cases, 88 pass / 3 fail ([XML](baseline-math.xml)); exact three filters
are 3 fail in both [baseline](baseline-three.xml) and [candidate](candidate-three.xml).
Configure/build logs and assertion output are retained. S1 does not repair or
waive this separate numerical gap and does not present the exclusion as a skip.

The supported product remains Decision 0283's scalar profile. Release/platform
acceptance, S2–S4 and complete record execution remain separate work.
