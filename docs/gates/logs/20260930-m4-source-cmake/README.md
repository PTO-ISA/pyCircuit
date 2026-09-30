# M4 explicit source-unit CMake build — 2026-09-30

Scope: approved C3-C explicit source DAG (§42, §220–224), using the existing
public compile/link commands. Base `ffef119c21c001d866c7dba5bac4006d6c6f751493`
plus the eight files in `candidate-files.json`. Product checkout:
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.

## Delivered

The checked-in fixture has a declaration source, child module, separate parent
root and unrelated module, each with one public compile producer. CMake declares
body/header/unit.json outputs, source depfiles, imported interfaces/receipts,
Python driver sources, native helpers and configured toolchain inputs. The
parent consumes headers; link consumes the complete three-unit closure.
The linked design is `linked/parent.ac`, following its source stem.

Ninja pre-creates parent directories of declared outputs. The first real build
therefore failed when the publication protocol saw an unmanaged empty unit
folder. A fixture-local preparer uses only atomic `rmdir`: absent or nonempty
folders are left for publication, populated directories are never recursively
removed, and symlinks in the destination/ancestors are rejected. There is no
public driver behavior change or adversarial filesystem-race guarantee.

## Independent verification

Implementation: `unit_pair_fix`, gpt-6-luna/high. Independent tests:
`cmake_source_tests`, separate gpt-6-luna/high. PM owns integration/docs and added the final clean/rebuild regression
after the test-author resumption hit the tool thread limit;
`other_agent_code_review`, gpt-5.6-sol/high, reviews independently. Final verdict: [APPROVE](review.md), bound to all eight
current candidate hashes.

- `initial-failure.log/xml/command.txt`: one genuine integration failure before
  empty-directory preparation, retained as debugging evidence. This is not a
  claim of failing-first coverage for all existing compiler semantics.
- `run-01.log/xml/command.txt`: the pre-clean-regression candidate passed one
  integration test; its byte binding is `candidate-files-run-01.json`.
- `clean-before.log` and `clean-before-state.txt`: PM reproduced normal Ninja
  clean leaving `.d` behind, then rebuild refusing the incomplete unit. The
  fixture now declares `.d` as BYPRODUCTS on its existing producer. The archived
  retry includes regeneration after that edit but no second clean; it demonstrates
  stale-directory refusal, not a frozen pre-fix run. Final clean/rebuild proof is
  run-02. Raw logs preserve tool-generated whitespace; source/doc diff checks
  exclude raw gate logs.
- `run-02.log/xml/command.txt`: **1 integration test passed**, no failures,
  errors or skips. It executes real CMake/Ninja/native helpers in paths with
  spaces, checks exactly one compile producer per source, a no-op build,
  selective child-source rebuild, imported header/receipt rebuilds, actual
  config invalidation, native/Python compiler graph dependencies, empty and
  populated/symlink output protection, header-only parent compile, and rejected
  incomplete link with byte-preserved existing final IR. The final run also checks
  clean removes all four unit artifacts, full rebuild/link succeeds, then a
  subsequent build does no work.
- Changed-file pre-commit: merge-conflict, EOF, whitespace, Ruff, Black,
  Markdown and API hygiene passed. YAML had no applicable files.
- Author ran the README configure/build and direct compile/link commands on
  fresh disposable output directories; those terminal-only results have no
  separate archived raw log. The independent test is the archived execution
  evidence. The README supplies the reproducible commands.

No compiler, runtime or product Python files changed. Existing checkout-built
native helpers were used; no native rebuild or broad semantic suite was run
for this fixture-only change. Current-host scope is macOS arm64/CMake/Ninja;
no Windows, install, C++/RTL execution or performance claim.

## Migration boundary

This completes the explicit AC compile/link build slice of M4, not all M4 or
V45. Source-owned C++ groups, independent C++ TUs, generated.json and the full
build/run delivery remain outstanding. Public new emit stays in M5. M2's bounded
core acceptance remains closed. First-class system/expect revision B proposals
remain unapproved and are not implemented here.
