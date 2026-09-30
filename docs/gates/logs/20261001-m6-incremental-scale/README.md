# M6-02 incremental source-unit build and scale baseline

Accepted: 2026-10-01, macOS arm64. Baseline:
`477beae8ee51e6d6d393f2c658ba5d8343eeeff6`.
Candidate aggregate:
`619fe3ae39b5624e7429080f9015b5143acc3f8c9f904b99da3d67c024ae5e4e`.
Ten-file binding is in `candidate-manifest.json`; installed `_emit.py` bytes were
checked equal to the source. Native compiler/runtime were built and installed
from this checkout; no binaries came from another worktree. The installed Git
metadata binds baseline HEAD, while the manifest binds this dirty candidate.

| Gate | Outcome | Evidence |
| --- | --- | --- |
| Full shared/distinct matrix | 6 cases passed, exit 0 | measurement.txt; measurement-summary.json; raw/ |
| PM focused M6 | 10 passed, zero skips | focused.txt; focused.xml |
| Independent tests | 10 passed on each Python 3.12.12/3.14.6 | independent-py312.txt; independent-py314.txt |
| Independent Sol review | APPROVE; 10 passed on Python 3.12 | review-r2-focused.txt; review-r2-ruff.txt; review document |
| Existing public emit/RTL/model ABI | 23 passed, zero skips | regression.txt |
| All-files hooks | passed | precommit.txt |
| Strict MkDocs | passed | docs.txt |

`measurement-summary.json` is a bounded projection of the disposable final raw
report. It retains all inventories, phase timings/exit statuses, command counts
and classified sources, invalidation paths/content/mtime flags, determinism and
oracles; repetitive command strings and per-artifact byte deltas are omitted.
Selected raw Ninja/runner logs are retained unmodified in `raw/`. `report-binding.json`
records both report hashes as gate evidence, never as product identity or ABI.
The complete raw report can be regenerated with the command below.

## Observed results

Seconds, wall clock including process startup, four build jobs, unisolated host;
these values are observations without thresholds or speed guarantees.

| Axis / size | Source producers | Implementation CPP TUs | Cold compile+link | CPP emit | RTL emit | CPP model build |
| --- | --- | --- | --- | --- | --- | --- |
| Shared / 1 | 3 | 2 | 0.446 | 0.181 | 0.178 | 0.656 |
| Shared / 16 | 3 | 2 | 0.473 | 0.193 | 0.192 | 0.653 |
| Shared / 64 | 3 | 2 | 0.577 | 0.238 | 0.237 | 0.706 |
| Distinct / 1 | 3 | 2 | 0.511 | 0.196 | 0.197 | 0.738 |
| Distinct / 8 | 10 | 9 | 1.048 | 0.453 | 0.449 | 1.780 |
| Distinct / 32 | 34 | 33 | 2.164 | 1.107 | 1.114 | 6.151 |

Each model target also compiles one glue TU; DUT and system targets compile
separately, so distinct-32 executes 34+34 C++ compilations, not merely 33.
All graph/emit/model no-ops execute zero compiler/linker commands and preserve
artifact bytes/mtimes. A leaf edit compiles its source and the root, leaving
independent sibling units unchanged. Type/interface and the dedicated toolchain
stamp invalidate all source producers. Full emit then republishes the generated
bundle and conservatively rebuilds all C++ targets. This is an optimization
opportunity, not a selective backend-cache guarantee.

Every size passes a literal per-instance/topic/value three-epoch C++ oracle.
Distinct-32 additionally builds/runs actual RTL with 96 reports, one TERMINATED
Result at epoch 3, and null error. Both targets emit from the same saved final IR;
fixtures use explicit Python declarations and source-owned compile commands.
Static Ninja queries prove dependencies, but are never counted as executed work.

## Corrections and review history

- Missing counter depfile/payload byproducts caused clean/rebuild failure;
  `clean-rebuild-before.txt` records it. Exact byproducts/dependencies and
  empty-tree-only preparation close it; `clean-rebuild-after.txt` and the final
  example regression prove byte restoration and retained control/lock identities.
- Verilator 5.044 split space-containing list paths in generated CMake.
  `spaces-before.*` records independent failing-first configure exit 1. Final
  independent tests build/run in-place with source and build directories containing
  spaces. The JSON-list quoting helper is restored after configuring the target.
- Review round 1 rejected candidate aggregate
  `30ca0958f5382441c7e7d9dce32ae4f58001c9cc35f7b4b8265d8d505c1250bf`:
  Python 3.12 direct helper execution imported sibling `types.py` through `pathlib`.
  `review-r1-fail.txt` records 7 passes/2 failures. The final `os.path` repair and
  symlink-plus-`..` preservation pass both independent interpreter runs.
- An initial PM product-regression invocation omitted private harness variables:
  5 fixture setup failures, 18 passes. Correcting the environment, with no emitter
  code change, yielded the archived 23/23 result; it was not treated as a defect.

## Reproduction

Working directory: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Tool versions are in the measurement metadata: LLVM/MLIR 22.1.8, Apple Clang 21,
CMake 4.2.1, Ninja 1.13, Verilator 5.044. PM pytest uses Python 3.14.6/pytest 9.0.2;
independent test/review also uses repo `.venv` Python 3.12.12.

```sh
cmake --build .pycircuit_out/m5-root -j 4
cmake --install .pycircuit_out/m5-root
export PYCIRCUIT_M6_PREFIX="$PWD/.pycircuit_out/m6-02-install"
export PYCIRCUIT_COMPILER_INSTALL="$PYCIRCUIT_M6_PREFIX"
export PYCIRCUIT_NATIVE_BUILD="$PWD/.pycircuit_out/m5-root"
export ACIR_SOURCE_UNIT_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-design-harness"
export ACIR_CPP_SOURCE_PARTS_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-cpp-source-parts-harness"
python3 flows/tools/measure_m6_build.py --prefix "$PYCIRCUIT_M6_PREFIX" --output-dir .pycircuit_out/m6-02-measure/final-r2
python -m pytest tests/system/test_m6_incremental_build.py tests/system/test_m6_example_build.py tests/unit/test_m6_measurement.py tests/unit/test_m6_prepare_output.py -q
python -m pytest tests/system/test_m5_public_emit.py tests/system/test_m5_source_rtl.py tests/system/test_m5_model_abi.py -q
pre-commit run --all-files
mkdocs build --strict
```

For a new revision, configure/build/install from that checkout so prefix Git
metadata matches its HEAD; use a fresh absent output directory rather than reusing
the archived run directory. The initial native configure/install used
`CMAKE_INSTALL_PREFIX=$PWD/.pycircuit_out/m6-02-install`; no fresh native rebuild
was needed for this Python/example-only correction.

RSS, isolated throughput, scheduling/reordering, other platforms and complete
SDK/fault matrices are outside this packet. Unicode generated-source/build host
directory names still hit invalid JSON octal escapes in local Verilator 5.044;
no arbitrary Unicode host-directory guarantee is claimed. This does not alter
SourceOwner/name legalization or M6-01's tested Unicode installation prefix.
SYSTEM/EXPECT B remains unapproved. This closes M6-02, not all M6 or M7.
