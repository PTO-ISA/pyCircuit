# M6-03 first-publication recovery evidence

Accepted: 2026-10-01, macOS arm64/POSIX. Baseline:
`5b0d610d798fca8be7188ec10674355e07dbab47`. Production bytes remain unchanged;
`candidate-manifest.json` binds two test/launcher files, and
`toolchain-metadata.json` binds the fresh native build from this checkout.

| Gate | Result | Evidence |
| --- | --- | --- |
| Fresh build/install | exit 0 | build.txt; toolchain-metadata.json |
| PM system matrix, Python 3.14 | 5 passed, zero skips | system.txt; system.xml |
| Independent Sol review/run, Python 3.12 | 5 passed | independent-review.txt |
| Publication/FS/driver regression | 131 passed, 3 Windows-only skips | regression.txt; regression.xml |
| Repository hooks and strict docs | passed | precommit.txt; docs.txt |

Five pytest functions include fifteen new final/CPP/RTL first-publication
SIGKILL rows, ten extra bundle follow-up writer interruption/recovery sequences,
and retained earlier 24 replacement, five source-first, three reentrant recovery
rows plus the waiting reader/writer case. Scenario counts are not pytest counts.
Every initial crash must emit the exact `crashed-at:<point>` and return -SIGKILL.
The marker is in a test-only launcher and adds no public fault interface.

| First-publish point | Crash state | Required recovery |
| --- | --- | --- |
| after_journal_preparing | preparing, no destination/previous or completed stage | absent |
| after_stage_complete | preparing, complete stage only | absent |
| after_journal_prepared | prepared, complete stage only | absent |
| after_previous_saved | unreachable with no previous | N/A, never PASS |
| after_destination_installed | prepared, complete destination only | absent |
| after_journal_committed | committed, complete destination only | retain exact new bytes |

Final precommit readers fail specifically with `published stable artifact is
missing or unsafe`, leave destination/consumer output absent, and clear rollback
state. A committed final reader emits verified expected CPP, and a later writer
completes pending cleanup. CPP/RTL writer recovery is inspected while paused at
its next preparing hook before it can republish: absence before commit, retained
bytes after commit. The pause is then killed; a fresh public writer recovers and
republishes. Existing control owner/lock inode identity remains stable throughout.

Coverage expansion required no product fix, so no fabricated failing-first
product claim is made. Test-development mistakes in quoted canonical owner and
missing-final diagnostic were corrected before frozen review. Post-Black files
were independently rerun; no assertion was deleted or weakened.

## Reproduce

Working directory: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
LLVM/MLIR 22.1.8; fresh build from current checkout, no foreign/stale toolchains.

```sh
export LLVM_DIR=/opt/homebrew/opt/llvm@22/lib/cmake/llvm
export MLIR_DIR=/opt/homebrew/opt/llvm@22/lib/cmake/mlir
export PYC_BUILD_DIR="$PWD/.pycircuit_out/m6-03-root"
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/m6-03-install"
PYC_BUILD_TESTING=ON CMAKE_BUILD_PARALLEL_LEVEL=4 bash flows/scripts/pyc build --build-dir "$PYC_BUILD_DIR" --install-prefix "$PYC_TOOLCHAIN_ROOT"
export PYCIRCUIT_NATIVE_BUILD="$PYC_BUILD_DIR"
export PYCIRCUIT_COMPILER_INSTALL="$PYC_TOOLCHAIN_ROOT"
export PYCIRCUIT_M6_PREFIX="$PYC_TOOLCHAIN_ROOT"
python3 -m pytest tests/system/test_m6_publication_process_recovery.py -q
python3 -m pytest tests/unit/test_publication.py tests/unit/test_publication_fs.py tests/unit/test_driver_commands.py -q
```

For another revision use a new output tree/configure so metadata matches HEAD.
The fixture derives all three native helper paths from that explicit build.
Production-source/hook definitions are unchanged; only the test-only marker and
new checks were added. The three unit skips require Windows rename/flush,
reparse metadata or directory-handle APIs; no platform skip counts as proof.

Independent review is in `docs/reviews/20261001-m6-first-publication-review.md`.
This closes the stated macOS first-publication gap, not Windows/network FS/
power-loss, full SDK/platform matrices, M6 overall or stable release readiness.
