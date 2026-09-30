# M5 cutover evidence

Run: 2026-09-30 through 2026-10-01 (Asia/Shanghai). Baseline: `a21bb596`.
Product byte binding: `8ae474b2a0c98d2909658d502f6c6763a20335b5754d7bba4d132bfaae8c1690`,
with the complete path/byte inventory in `product-manifest.json`.
This is the bounded scalar/default-clock/portless-root/empty-static profile,
not delivery of all M3/M6 capabilities or a published release.

## Results

| Lane | Result | Raw evidence |
| --- | --- | --- |
| Lightweight unit | 309 passed, 3 Windows-only skipped, 79 system cases deselected by marker | unit.xml, unit.txt |
| System / source / both backends / installed wheel | 406 passed, 2 agreed M6 V44 cases deselected | system.xml, system.txt |
| Native | 20 CTest targets passed, no failed targets | native.xml, native.txt |
| Documentation source and example DAG | 2 passed, 5 unit cases deselected by marker | docs-example.xml, docs-example.txt |
| Hooks | all hooks passed | precommit.txt |
| MkDocs, current retirement inventory, repository policy, SDK schemas | passed | commands below |

The three skips require real Windows rename/reparse/directory-handle APIs.
The two V44 exclusions retain the prior M4 scope boundary for actual schedule
permutations/source reordering; neither is evidence of parallel execution.
The wheel case installs one distribution, uses its prefix launcher outside its
venv, and builds the copied per-source example. The source-map tests include
empty/facade/Unicode owners, unused/private declarations, canonical/fused/unknown
locations, exact derived map paths and publication preservation.

## Reproduction

Configure/build the current checkout with root CMake, exact LLVM/MLIR 22.1.8,
`PYC_BUILD_COMPILER_DEV=ON`, `PYC_BUILD_TESTING=ON`, and a fresh install prefix.
No compiler or generated library was copied from another checkout.

```sh
export PYCIRCUIT_NATIVE_BUILD="$PWD/.pycircuit_out/m5-root"
export PYCIRCUIT_COMPILER_INSTALL="$PWD/.pycircuit_out/m5-candidate-install"
export ACIR_SOURCE_UNIT_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-design-harness"
export ACIR_CPP_SOURCE_PARTS_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-cpp-source-parts-harness"
export ACIR_BACKEND_CLOSURE_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-backend-closure-harness"
export MLIR_OPT=/opt/homebrew/opt/llvm@22/bin/mlir-opt
python -m pytest tests/unit -m unit -q
python -m pytest tests/system tests/unit/test_m5_source_map_validation.py -q --deselect tests/system/test_unified_register_backends.py::test_v44_schedule_permutations_preserve_values_errors_and_events --deselect tests/system/test_unified_register_backends.py::test_v44_source_reorder_compares_explicit_semantic_identity
python -m pytest tests/unit/test_example_layout.py tests/unit/test_tutorial_snippets.py -m system -q
ctest --test-dir "$PYCIRCUIT_NATIVE_BUILD" --output-on-failure -j 4
pre-commit run --all-files
mkdocs build --strict
python flows/tools/check_m5_retirement.py --install-root "$PYCIRCUIT_COMPILER_INSTALL"
python .github/scripts/validate_repo_management.py
uv run --no-project --offline --with jsonschema python packaging/sdk/check_contract.py
```

Build the platform wheel first with `packaging/wheel/create_wheel.py`; set
`PYCIRCUIT_M5_WHEEL` to that exact artifact when it differs from the test's
`.pycircuit_out/m5-wheel` default. SDK schema tooling uses the existing release
validation dependency `jsonschema`; the installed product has no Python
third-party runtime dependency.

## Review corrections

Independent review found and the candidate fixes: source-map path rebinding;
Unicode/formal/generated RTL identifier collisions; output bootstrap inside an
input publication control tree; leaked runtime exports; incomplete CompilerDev
headers/includes; dangling Runtime tool metadata; the wheel's prefix launcher;
retired CMake aliases, SDK cost payloads, lit runners, benchmark/tool callers,
and stale CI validator invocations. Independent tests retain the supported
oracles and cover these boundaries. No old semantic engine was restored.

Historical source-file evidence removed from the active tree is pinned by
baseline blob links and byte hashes in `retired-evidence.md`. This index is not
fresh execution evidence for those superseded implementations.

## Limits

No SYSTEM/EXPECT revision B implementation, memory/CDC/four-state/complex static
parameters, parallel scheduler or generated-line source map is claimed.
No release tag/PyPI publication or remote branch-protection mutation was made.
The local required-check name update needs reconciliation when the repository
administrator rolls the candidate into the protected default branch.
Final reviews and PM acceptance are recorded separately in docs/reviews and
the M5 work item; a passing intermediate run is not independently accepted code.
