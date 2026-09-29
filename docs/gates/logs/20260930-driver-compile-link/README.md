# C3 public driver: `pycircuit compile` and `pycircuit link` (2026-09-30)

Scope: expose the already-approved C3-C `compile` and `link` commands of the
single `pycircuit` driver over the existing semantic chain. Python captures one
stable snapshot, hands explicit inputs to the existing private native helper, and
publishes the verified result under the existing shared publication protocol. No
second semantic chain, no MLIR parsing in Python, no new public interface.

## Approval

Approval record: `docs/rfcs/migration/approvals/c3-compile-link-driver.md` on the
planning branch `codex/gfsim-migration-governance` (commit `ef5248eb`). It is not
present on this branch, which carries only the implementation, so it is cited by
path rather than by a link that would not resolve here. The approved interface is
C3-C (`c3-driver-runtime.md`, SHA-256
`0c476ced27519cf93427a89f77b9348d388e96db57fb183790144d24118b1170`), verified
unchanged before this packet started. The user approved, in this round:

- delivering `pycircuit compile` and `pycircuit link` only;
- **not** introducing C3 `emit`, which stays for the M5 hard break because the
  name is occupied by the legacy route;
- `link --parameters` failing closed on a non-empty binding array;
- adding the private `--entry-owner-out` channel to `acir-design-harness`, the
  same additive pattern as the previously approved `--deps-out`.

Not approved, and not implemented here: first-class `ac.system` (C2-SYSTEM
revision B `db81dbc8…`), the `ac.expect` per-field freeze (C2-EXPECT revision B
`e8f287e7…`), static parameter specialization, per-source C++ file groups,
`generated.json`, and any SDK/runtime/manifest change.

## Baseline and file whitelist

| Item | Value |
| --- | --- |
| Implementation baseline | `d6233d1e` |
| Planning baseline | `4a4a0443` |
| Shared build | `.pycircuit_out/w10-pm/build` (single integrator) |

| File | Writer | Change |
| --- | --- | --- |
| `python/pycircuit/src/pycircuit/_driver.py` | this packet (new) | the two public command implementations |
| `python/pycircuit/src/pycircuit/cli.py` | this packet | two subparsers and their handlers only |
| `compiler/acir/tools/acir-design-harness/acir-design-harness.cpp` | this packet | the additive private `--entry-owner-out` channel |
| `tests/unit/test_driver_commands.py` | independent test author | unit-level coverage |
| `tests/system/test_driver_compile_link.py` | independent test author | end-to-end coverage |

Not modified: ODS and any op/type/attribute schema, the emitters, the numeric
classifier, runtime, SDK, packaging, `generated.json`, the publication
transaction semantics, the legacy `build`/`emit`/`inspect`/`sidecar` behaviour,
AGENTS, historical approval records and their hashes.

## Commands

Exactly the approved C3-C surface, with no extra option:

```text
pycircuit compile -c <one-source.py> --source-root <root>
  [--package-prefix <dotted-prefix>]
  [-I <interface-unit-dir>]... -o <unit-dir> [--replace]

pycircuit link <unit-dir>... --top <qualified-module>
  [--parameters <bindings.json>] -o <program.ac> [--replace]
```

`compile` publishes the closed four-file set through the private
`_compile_source_unit` entry accepted in the previous packet; this packet adds no
semantics to it.

## Design of `link`

**One command uses one complete lock set.** All unit directories are shared
inputs and the program artifact is the exclusive output, acquired by a single
`_publication_lock_set` that covers snapshotting, the native link, and
publication. Every unit is read with `_read_full_source_unit`, so link consumes
the full body/header closure under the lock, unlike compile, which is satisfied
by a managed interface.

**The linker never reopens a provider file.** Each in-lock snapshot is written to
a scratch copy and the helper is given those copies, so the forbidden pattern —
validate, release, re-read during compilation, then take a separate output lock —
is structurally impossible here.

**Owner discovery is never the authority.** Unit owners are read before locking
only to name the expected inputs; the lock set re-validates each owner against
the journal and re-validates the artifact under the lock. The program's
publication owner is not guessed from module names: the private
`--entry-owner-out` channel reports the root's declared `ac.source_owner` and its
canonical definition, which only the linker knows. This is what makes
`--replace` mean "same root", as C3-C §181 requires.

**Publication validator is intrinsic.** The publication protocol validates an
existing artifact with the same validator it uses for the new one, so the
program validator checks the file itself (regular, non-symlink, non-empty)
rather than comparing against the newly linked bytes. A comparative validator
would reject every genuine re-link.

**Fail closed where the native side cannot answer.** `--parameters` with a
non-empty array is rejected with an explicit diagnostic because the linker has no
static-parameter entry point yet; omitted and explicitly empty are equivalent to
"no bindings", matching C3-C §53.

**`--replace` touches only what this driver published.** When the destination
exists and `--replace` is set, `link` requires the publication control directory
to already exist before the lock set can bootstrap one, and refuses otherwise:

```text
pycircuit link: refusing to replace a path this driver did not publish: <path>
```

This closes the C3-C §143 consequence that a pre-existing user file at the output
path would otherwise be overwritten: the file is left byte-identical and no
control directory is created. What remains open is narrower and is declared
below — the previous *owner* cannot be compared, because a committed program
publication persists no owner.

## Declared gaps

These are recorded rather than hidden; none of them is faked or worked around:

- **C3 `emit` is not delivered.** The subcommand name is occupied by the legacy
  route, and C3-C forbids aliases while the approved plan retires the old route
  only at the M5 hard break.
- **Static parameter specialization is not implemented.** `link --parameters`
  accepts only the empty array.
- **`--replace` cannot compare the previous program owner.** `program.ac` carries
  no Python-readable owner record and there is no standalone native verify-only
  entry, so relinking a genuinely different root onto a path this driver already
  published is still accepted; only the "did this driver publish it at all" part
  of C3-C §143/§181 is enforced (see below). Closing the owner comparison needs a
  native verify/owner-read entry.
- **`emit`'s per-implementation-source `.hpp/.cpp` groups and `generated.json`
  are not produced.** The current C++ backend returns one monolithic artifact;
  splitting it in Python to fake per-source groups is explicitly rejected.
- **A `@system` root cannot be linked by this command.** The private
  `@system ⟹ testbench` role rule is still in force; removing it is part of
  C2-SYSTEM revision B, which is not approved. Link therefore supports the plain
  portless `@module` root that C3-C §55 describes.

## Verification

Environment is recorded in `lane-env.txt`. Lane results are in the table below;
`unit.xml`, `system-focused.xml`, `driver.xml`, and `transport-mlir.xml` are the
raw JUnit artifacts, and the counts were read by parsing the XML rather than the
pytest summary line.

| Lane | Log | Tests | failures | errors | skipped | exit |
| --- | --- | --- | --- | --- | --- | --- |
| unit (previous packet) | `unit.xml` | 174 | 0 | 0 | 3 | 0 |
| system focused (previous packet) | `system-focused.xml` | 105 | 0 | 0 | 0 | 0 |
| LLVM transport | `transport-mlir.xml` | 4 | 0 | 0 | 0 | 0 |
| driver (this packet) | `driver.xml` | 66 | 0 | 0 | 0 | 0 |
| CLI regression | `cli-regression.xml` | 60 | 0 | 0 | 0 | 0 |
| masked-next (remaining helper consumer) | `masked-next.xml` | 15 | 0 | 0 | 0 | 0 |

The pre-existing lanes were re-run after the helper change and the new
subcommands to show this packet does not regress them. Every consumer of
`acir-design-harness` in the tree is covered by one of these lanes; there is no
lit or script consumer whose option table could have been affected.

### Auxiliary evidence

| File | What it records |
| --- | --- |
| `cli-surface.log` | the C3-C approved syntax verbatim next to `compile --help` and `link --help`, showing the surface is option-for-option the approved one |
| `entry-owner-channel.log` | the positive (`--entry-owner-out` reports the ROOT source `counter.py`, not the declaration provider `types.py`), three native negatives, and the measured `--replace` owner gap |
| `precise-probes.log` | body and depfile deleted in place fail closed with nothing published; a symlinked output parent is a clean diagnostic; the compile/link closure contrast |
| `adversarial-probes.log` | duplicate source, two directories declaring one source, an unmatched `--top`, an interface-only unit linked alone, a missing output parent, and two concurrent links to one destination |

### Defects found and fixed during this packet

1. A symlinked output path chain raises `_PublicationFileSystemError`, which is a
   `RuntimeError` subclass and was not in the CLI handlers' catch tuple, so both
   new commands printed a full Python traceback instead of a one-line diagnostic.
   Fixed by catching that type in both handlers; `precise-probes.log` records the
   clean diagnostic and the zero published entries.
2. The private receipt reader reports any read failure, including a missing file,
   as `source-unit receipt is not stable UTF-8`. That is misleading on the new
   public surface, so both commands now pre-check each unit directory through one
   shared helper and report either `... is not a directory` or `... is not a
   published source unit (no unit.json)`. The independent test author confirmed
   the message contains neither `UTF-8` nor `encoding`.

The private modules' exception contracts and diagnostics were not changed — the
public commands now handle and pre-empt what the private layer raises. One case
remains a private-layer message: a symlinked *unit* path reports `unmanaged
publication input is not accepted` rather than naming the symlink.

The driver lane is 43 unit tests (`tests/unit/test_driver_commands.py`, fake
helpers, CLI driven in-process) plus 23 system tests
(`tests/system/test_driver_compile_link.py`, real helpers, CLI as a subprocess so
exit status, the single-line stderr and the absence of a traceback are
process-level facts). The last system test covers the `--replace` guard that
closes D1 below. The independent test author reports 8/8 mutation checks detected
on throwaway copies of the package for the first revision and 3/3 for the fix —
guard removal, making `-c` optional, and renaming `-o` back to a long spelling.

### Findings the test author reported

- **The `--replace` owner asymmetry is real and measured.** `compile` rejects a
  different package with `--replace` because the unit receipt embeds its owner;
  `link` accepts a different root onto the same program path because a committed
  single-file publication persists no owner. This is the declared gap above, now
  confirmed by an independent second measurement.
- `owner.json` is written with sorted, compact keys
  (`{"destination":…,"kind":…}`), which equals the C3-C object as JSON but is not
  byte-identical to the literal in the RFC. The protocol compares parsed values.
- On any link failure the output's control directory (`owner.json` + `lock`) is
  created before the work and no journal, stage or artifact survives — the
  documented bootstrap behaviour.
- A symlinked *unit* path is rejected as `unmanaged publication input is not
  accepted` because the control directory derives from the given path name;
  fail-closed, but the message names the wrong cause.
- The depfile for a consumed provider names `<provider>.interface.ac` and
  `<provider>/unit.json` and never `<provider>.ac`, which is the measured
  consumed-closure behaviour from the previous packet.

## Independent review

Requested from a separate instance; not self-signed.

**First review, of `df3f2283` + `57b1ed43`: FAIL — declaration and governance, not
implementation.** The reviewer reproduced every functional claim independently
(no MLIR parsing in Python, scratch-copy consumption, the output lock held across
the native step, the root-owner report surviving renamed unit directories, the
overlay hashes, the raw-XML lane counts, the additive C++ diff) and failed the
packet for understating a real C3-C violation.

| # | Finding | Disposition |
| --- | --- | --- |
| D1 | **Material.** `link --replace` overwrote any pre-existing non-empty regular file at the output path, including a user file the driver never published — which C3-C §143 forbids outright — while the packet declared only the §181 owner-comparison gap. | **Closed, not merely declared.** When the destination exists and `--replace` is set, `link` now requires the publication control directory to pre-exist and otherwise refuses, leaving the file byte-identical and creating no control directory (`replace-guard.log`). |
| D2 | Low. `compile` also accepted `--source`, `--interface-unit` and `--output`, which are not in the approved syntax block, contradicting this README's "no extra option". | The three aliases were removed; the surface is now literally the approved one, and `cli-surface.log` is regenerated from it. |
| D3 | Low, documentation. The approval citation linked to a path that does not exist on this branch. | Cited by path with its planning-branch commit instead. |
| D4 | Trivial. `_PublicationError` was imported but unused in `_driver.py`. | Now used by the replace guard. |
| D5 | Trivial. A docstring claimed the published program "is re-verified when read", which nothing in the tree does. | Corrected. |

**Re-verification requested** for the fix commits and the evidence commit carrying
this table. The verdict is to be appended before the packet is accepted.
