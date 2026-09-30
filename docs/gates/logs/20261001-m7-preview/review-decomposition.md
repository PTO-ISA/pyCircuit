# M7-01 read-only decomposition (baseline 25152f9c630de6d7d019edcd6c9ed09ce1bebbfc)

## Verdict before implementation

Blocked from preview acceptance until the inherited gate defects below are repaired and the frozen candidate is rerun. Scope remains the approved scalar compile/link/emit profile on local macOS arm64. No release/tag/publication claim is available from this packet.

## Actionable inherited defects

1. **Root CMake presets still select the retired product.** `CMakePresets.json:17` sets removed `PYC_BUILD_MLIR_TOOLS`; `CMakePresets.json:25-27` exposes `release-tools` and requests deleted `pycc`/`pyc-opt` targets. The current root options are `PYC_BUILD_COMPILER_DEV`, `PYC_BUILD_RUNTIME_LIB`, `PYC_BUILD_TESTING`, and `PYC_INSTALL_PYTHON` (`CMakeLists.txt:6-11`). Repair the configure preset to those current options and replace the stale build preset with a current all-target build. This is a hard-break developer/build correction, not a compatibility alias.

2. **The retirement scanner has a false-negative hole.** `flows/tools/check_m5_retirement.py:15-30` excludes `CMakePresets.json`, and its retired compiler regex at lines 53-57 only recognizes CMake command syntax, not JSON target arrays. The scanner currently exits 0 while the retired preset remains. Add the root preset to production roots, explicitly reject `PYC_BUILD_MLIR_TOOLS` and retired preset targets, and pin that coverage in an independent unit test.

3. **Accepted M6 gates are not part of the current release/preview closure.** `.github/workflows/release.yml:93-101` runs the four M5 closure scripts plus retirement only. `flows/scripts/run_semantic_regressions_v6.sh:22-44` contains no M6-01/M6-02 system tests. The accepted packets require publication recovery, moved-prefix, clean/rebuild, space-path RTL, source-unit invalidation and TU/oracle coverage. M7 local preview must run those exact tests from the current build/install; it must not rely on old M6 evidence.

4. **The moved-prefix gate is bound to an obsolete build directory.** `tests/system/test_m6_relocated_compiler.py:20` hardcodes `.pycircuit_out/m5-root`, and lines 97-118 install from it. This cannot validate the forthcoming `.pycircuit_out/m7-01-root` candidate and risks accepting stale binaries. Make it honor `PYCIRCUIT_NATIVE_BUILD`, consistent with the other system gates. For the incremental/example gates, bind `PYCIRCUIT_M6_PREFIX` to `.pycircuit_out/m7-01-install`.

5. **Formal release workflow is not an M7-01 acceptance gate.** The checkout and SDK metadata still declare 6.1.0 / `v6.1.0`, while local `v6.1.0^{}` peels to `d4926615bd0e90978a5f8135d320dc702bb0b81a`, not this baseline. `release.yml:57-65` requires that version and rejects an existing tag; lines 110 onward require Linux/macOS/Windows candidates, and lines 565 onward tag and publish. M7-01 is local macOS preview only: do not dispatch this workflow or claim its platform/publication acceptance. A future formal release needs a separately authorized version/tag/platform packet.

6. **Several active release/packaging descriptions still claim retired contents.** `packaging/sdk/create_platform_manifest.py:551-552` and `.github/workflows/publish-pypi.yml:9-11` say the wheel carries both frontends and both compilers; `release.yml:44` calls the job “Full AC/PYC Closure”. The executable package code is current-route-only, so correct these descriptions. Keep the dated 6.1.0 changelog entry as history; add an Unreleased/current-preview note instead of rewriting released history. Also reconcile `docs/development/pycircuit-modernization-tests.md:94`, whose current-closure list still names removed `run_agentic_circuit.sh`, with `tests/unit/test_gate_topology.py:208-213`, which correctly forbids that script in release.

## Selected local macOS preview matrix

All commands must bind the frozen candidate and use only the fresh current checkout build/install.

1. **Fresh toolchain**
   - `PYC_BUILD_TESTING=ON bash flows/scripts/pyc build --build-dir "$PWD/.pycircuit_out/m7-01-root" --install-prefix "$PWD/.pycircuit_out/m7-01-install"`
   - Record `toolchain-metadata.json` and assert its full Git SHA equals the candidate baseline/commit.

2. **Static/current-route closure**
   - `python3 .github/scripts/validate_repo_management.py`
   - `SKIP=pyc-api-hygiene pre-commit run --all-files`
   - `python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md`
   - strict decision status using the same flags as `release.yml:91`
   - `pytest tests/unit -m unit`
   - `mkdocs build --strict`
   - `python3 flows/tools/check_m5_retirement.py --install-root "$PWD/.pycircuit_out/m7-01-install"`

3. **Current source/backend/runtime closure**
   - Export `PYC_BUILD_DIR=.pycircuit_out/m7-01-root`, `PYC_TOOLCHAIN_ROOT=.pycircuit_out/m7-01-install`, `PYCIRCUIT_NATIVE_BUILD=.pycircuit_out/m7-01-root`, `PYCIRCUIT_COMPILER_INSTALL=.pycircuit_out/m7-01-install`.
   - Run `run_examples.sh`, `run_sims.sh`, `run_sims_nightly.sh`, and `run_semantic_regressions_v6.sh` from the candidate.
   - Preserve exact test inventory; reject empty collection and skips outside documented platform skips.

4. **Accepted M6 hardening on the same candidate**
   - `pytest tests/system/test_m6_publication_process_recovery.py tests/system/test_m6_relocated_compiler.py -q`
   - With `PYCIRCUIT_M6_PREFIX=.pycircuit_out/m7-01-install`, run `pytest tests/system/test_m6_incremental_build.py tests/system/test_m6_example_build.py tests/unit/test_m6_measurement.py tests/unit/test_m6_prepare_output.py -q`.
   - A fresh six-case scale measurement is not required to re-prove M6-02 unless measurement/generator/product bytes change; the system gates still prove the actual current build graph and RTL path.

5. **Disposable local packaging smoke**
   - Build one macOS-arm64 wheel from `.pycircuit_out/m7-01-install` under `.pycircuit_out/m7-01-review/`.
   - Run `tests/system/test_m5_wheel_install.py` with `PYCIRCUIT_M5_WHEEL` naming that exact wheel.
   - Inspect wheel entry points/package trees and installed bundled prefix for only `pycircuit`, the three private helpers, Runtime package, and no retired commands/namespaces.
   - This is a local preview artifact. Do not create SDK release indexes, tags, GitHub Releases, GHCR artifacts, or PyPI uploads.

6. **Evidence and final review**
   - Freeze changed product/gate/docs files; bind sorted path + NUL + SHA-256 + newline.
   - Archive commands, exit codes, inventories, candidate SHA, macOS/arm64/Python/CMake/Ninja/LLVM/MLIR/Verilator identities, and skipped/unrun platform scope.
   - Final verdict is bounded to a reproducible local macOS scalar-profile preview. It does not close all M6/M7, widen source semantics, or establish Linux/Windows/publication support.
