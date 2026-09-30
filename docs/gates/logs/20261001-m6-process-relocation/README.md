# M6-01 process recovery and moved compiler prefix

Accepted: 2026-10-01. Product baseline:
`d351079b4e4f2ba254cc57950f27d59c51dd7c38` (unchanged).
Test/fixture binding:
`2d65e3c76ad557a3de030a0cdbbc53b4518fa5007c4705a789de4dce44f9dc45`.
`test-manifest.json` binds the two tests, crash runner and six fixtures.

| Gate | Result | Evidence |
| --- | --- | --- |
| M6 focused system | 5 passed, zero skips | system.xml, system.txt |
| Publication/filesystem/driver regression | 131 passed, 3 Windows-only skips | regression.xml, regression.txt |
| Repository hooks | all passed | precommit.txt |
| Strict MkDocs | passed | recorded command below |

V48 includes 24 replacement crashes (compile/link/CPP emit/RTL emit × six
high-level publication points), five reachable first-publication compile
points, three interruptions during rollback recovery, and a paused-writer
lock-blocking case. Each crash is real self-SIGKILL with negative signal exit.
Journal/path layouts and exact artifact bytes follow C3. Shared committed reads
retain new data and may leave cleanup pending; a later writer completes cleanup.
The first-publication matrix is intentionally source-unit-only in this packet;
new final/bundle first-publication expansion remains later V48 coverage.

V47 installs from the current checkout, renames the full prefix to a path with
spaces/Unicode, proves the original absent, and uses an isolated no-pycircuit
Python environment. It clears ambient Python, compiler, loader and CMake paths,
then performs per-source compile/link, removes sources/units, emits both targets
from the saved final and builds/runs them against only the moved Runtime package.
The literal event/statistics oracle and resolved Mach-O dependency/RPATH checks
pass. The test also passes under Python 3.12.12 and 3.14.6; this does not claim a
complete interpreter or OS matrix.

## Reproduction

```sh
export PYCIRCUIT_NATIVE_BUILD="$PWD/.pycircuit_out/m5-root"
export ACIR_SOURCE_UNIT_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-design-harness"
export ACIR_CPP_SOURCE_PARTS_HARNESS="$PYCIRCUIT_NATIVE_BUILD/bin/acir-cpp-source-parts-harness"
python -m pytest tests/system/test_m6_publication_process_recovery.py tests/system/test_m6_relocated_compiler.py -q
python -m pytest tests/unit/test_publication.py tests/unit/test_publication_fs.py tests/unit/test_driver_commands.py -q
pre-commit run --all-files
mkdocs build --strict
```

PM ran the gates with `/opt/homebrew/Cellar/pytest/9.0.2_1/libexec/bin/python`.
A fresh install is created by the relocation test itself; it uses no toolchain
from another checkout. The three regression skips require Windows rename,
reparse-point or directory-handle APIs on a real Windows host.

Independent Sol high review APPROVE is archived in
`docs/reviews/20261001-m6-publication-relocation-review.md`. Test-shape fixes
were made before acceptance (public argv extraction, path-dependent depfiles,
precise recovery layouts, committed-read cleanup semantics, dylib install IDs,
and framework-aware Python isolation). No product behavior was changed.

This accepts one current macOS-arm64 V47/V48 packet. Linux/Windows, actual
parallel simulation, broader first-publication/fault matrices, performance and
M7 release remain separate work. No publishing/tagging/admin policy action is
implied by this evidence.
