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

**`--replace` refuses a destination with no publication control directory
naming it.** When the destination exists, or is a symlink (including a dangling
one, which `Path.exists()` cannot see), and `--replace` is set, `link` requires
that control directory before the lock set can bootstrap one, and refuses
otherwise:

```text
pycircuit link: refusing to replace a path without a publication control directory naming it: <path>
```

**This is a typo and accident guard, not proof of authorship.** The control
marker has no secret and the control directory is also bootstrapped, because
C3-C §160 requires the lock set to bootstrap every exclusive output before the
work, by any earlier command that reached the lock set at that path and then
failed — a non-empty `--parameters`, an unpublished unit argument or a missing
output parent all fail earlier and seed nothing. So this guard does not establish
that the driver published the file it is about to replace, and the round-2 review
is right that the earlier wording claimed more than the check proves. What it
does establish, every case executed in `replace-guard.log`, is that none of these
is replaced and none creates a control directory where none existed:

- a foreign non-empty file, an empty file, a directory, a symlink, a FIFO, a
  control directory copied from another artifact (its marker names the wrong
  destination), an uninitialized control directory, a lock-only control
  directory, a symlinked control path and a control path that is a regular file
  are all refused with the artifact byte-intact;
- a dangling symlink is now refused by the guard too, so it no longer leaves
  behind the control directory that would authorise a later replacement;
- republishing a path this driver really did publish, and a first publication with
  `--replace`, still work. Publication *recovery* under the guard was exercised
  directly by the round-3 reviewer on seeded committed and prepared/previous
  leftovers (both recovered and republished, exit 0), and the publication
  package's own recovery suite is in the unit lane; it is not demonstrated in
  `replace-guard.log`.

Four residues remain and are declared below: the previous *owner* cannot be
compared, a file at a path this driver did publish but that was later modified is
not recognized as a corrupted artifact, a control directory bootstrapped by an
earlier command that reached the lock set at that path and then failed will
authorise a replacement of a user file placed there afterwards, and a hand-written
control marker opens the guard because it carries no secret.

## Declared gaps

These are recorded rather than hidden; none of them is faked or worked around:

- **C3 `emit` is not delivered.** The subcommand name is occupied by the legacy
  route, and C3-C forbids aliases while the approved plan retires the old route
  only at the M5 hard break.
- **Static parameter specialization is not implemented.** `link --parameters`
  accepts only the empty array.
- **`--replace` still cannot compare the previous program, in four ways.** All
  four have the same root cause — `program.ac` carries no Python-readable owner
  or receipt, there is no standalone native verify-only entry, and C3-C §158
  fixes the control directory's permitted entries to seven names so no
  "successfully published" marker can be added. First, a genuinely different root
  relinked onto a path this driver published is accepted, so §181's "same owner"
  is not enforced. Second, if the file at such a path is later replaced by
  something else, the driver cannot tell it from a corrupted artifact and
  `--replace` overwrites it. Third, a control directory left behind by an earlier
  command that reached the lock set at that path and then failed — which C3-C §160
  requires — will authorise replacing a user file placed there afterwards, with no
  forgery involved. Fourth, a hand-written control marker plus an empty `lock`
  opens the guard outright, because the marker carries no secret; that is a
  corollary of the invariant being stated in its general form rather than a
  separate hole. What *is* enforced is only that a publication control directory
  names the destination; that is an accident guard, not authorship. Every
  measurement is in `replace-guard.log`; closing any of the four needs a native
  verify/owner-read entry or an RFC change to the control directory.
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
this table.

## Independent review, round 2

Reviewed `0b35b2a2` (fixes), `8d56bbb0` and `159ce16e` (evidence) at HEAD
`159ce16e`, against C3-C `0c476ced…` (re-verified unchanged).

**Verdict: FAIL** — narrow, and unlike round 1 not an implementation defect.

Dispositions of the round-1 findings, each re-derived by the reviewer:

- **D1 — CLOSED, verified.** `_driver.py:218-229` requires an initialized
  publication control directory before the lock set can bootstrap one. The
  reviewer re-ran the original probe and the requested bypass set (empty file,
  directory, symlink, FIFO, control directory copied from another artifact,
  uninitialized control directory, lock-only control directory, symlinked
  control path, control path as a regular file): all exit 1 with the artifact
  byte-intact and no control directory created. Republishing a genuinely
  published path, first publication with `--replace`, and `--replace` recovery
  semantics all still work; `compile --replace` on a foreign directory is still
  rejected; a symlinked output parent is still a clean one-line diagnostic.
- **D2 — FIXED.** The three long aliases are gone; the live `--help` output,
  `cli-surface.log` and C3-C now agree option-for-option.
- **D3 — FIXED.** The approval record is cited by path with planning-branch
  commit `ef5248eb`, verified present on `codex/gfsim-migration-governance` in
  the planning repository. (The round-1 "does not resolve" finding was correct
  about this branch, where the record does not exist; it now does not need to.)
- **D4/D5 — FIXED.** `_PublicationError` is used by the guard; the
  `_validate_published_program` docstring no longer claims read-time
  re-verification.

**New finding N1 (material to the declaration, not to the code).** The packet
asserts at `README.md:102` that "`--replace` touches only what this driver
published" and at `README.md:134` that "what *is* enforced is that the path was
published by this driver at all". What is enforced is that a publication control
directory naming the destination exists. The reviewer demonstrated three
reachable counterexamples: (a) any command that fails at a fresh output path
leaves that control directory behind — the lock set bootstraps every exclusive
output before the work, as C3-C §160 requires — so a user file placed at that
path afterwards is destroyed by `--replace` with exit 0, with no forgery and no
prior successful publication; (a′) `--replace` onto a dangling symlink skips the
guard because `Path.exists()` follows symlinks, is rejected by the protocol, and
leaves the control directory that then authorises the same destruction; (b) a
hand-written marker plus an empty `lock` opens the guard. The residue declared
in `159ce16e` — a file at a path the driver *did* publish and that was later
modified — is itself accurate and was reproduced exactly; the enforcement claim
printed beside it is not.

The reviewer notes this residue cannot be closed in Python: C3-C §158 fixes the
control directory's permitted entries to seven names, so no "successfully
published" marker may be added, and there is no receipt or verify-only entry.
The packet must therefore state the invariant it actually has, e.g.
"`--replace` refuses any destination without a publication control directory
naming it", and record the bootstrap residue beside the other two. The guard's
diagnostic wording at `_driver.py:227` overstates the same check.

Evidence was found sound: overlay hashes 5/5 match HEAD, the worktree and
`0b35b2a2`; all six JUnit XMLs parsed independently give unit 174 (3 skipped),
system-focused 105, transport 4, driver 66 (43 unit + 23 system), CLI regression
60, masked-next 15, all with zero failures and errors; XML timestamps confirm the
lanes were genuinely re-run; the reviewer re-ran the driver lane (66, exit 0),
the focused system lane (105, exit 0) and the remaining four lanes (250 passed +
3 skipped, exit 0). Legacy `build`/`emit`/`sidecar` behaviour is intact
including a real legacy `emit` producing a `.pyc`.

Not verified: the 3/3 mutation-check claim (no artifacts); the shared build
against a fresh compile (the C++ is byte-identical to round 1); fault-injected
recovery combined with the new guard; the Windows lock backend.

### Round-2 disposition

N1 accepted. The claim was corrected rather than defended: the design section now
states the invariant the check actually establishes and calls it what it is — a
typo and accident guard, not proof of authorship. The diagnostic was reworded to
`refusing to replace a path with no publication control directory: <path>`, and
the guard now also triggers on a symlink destination, which closes (a′): a
dangling symlink is refused by the guard and no longer leaves behind the control
directory that authorised the later harm. (a) and (b) cannot be closed in Python
for the reasons the reviewer gives, and are now declared as the third residue
beside the owner and corrupted-artifact ones in "Declared gaps".

## Independent review, round 3

Reviewed `6228bcc4` (fix) and `ddbf0449` (evidence) at HEAD `ddbf0449`, against
C3-C `0c476ced…` (re-verified unchanged).

**Verdict: PASS.**

The reviewer re-derived every item rather than accepting the disposition:

- **(a′) closed, verified.** `_driver.py:218` now tests
  `destination.exists() or destination.is_symlink()`. `--replace` onto a fresh
  dangling symlink exits 1 with the new diagnostic, leaves the symlink intact and
  creates **no** control directory; previously the guard was skipped and the
  lock-set bootstrap left behind the directory that authorised the later harm.
  On POSIX the pair covers every occupied path.
- **D1's closure still holds.** The full bypass matrix was re-run against the new
  condition — foreign non-empty file, empty file, directory, valid symlink, FIFO,
  a control directory copied from another artifact, an uninitialized control
  directory, a lock-only control directory, a symlinked control path and a
  control path that is a regular file — all exit 1 with the artifact byte-intact
  and no control directory created. A control directory carrying a stray entry
  passes the guard and is then rejected by `_validate_control`, still fail-closed.
  Republishing, first publication with `--replace`, the no-`--replace` rejection,
  the `compile` foreign-directory rejection and the symlinked-parent diagnostic
  are all unchanged. Publication recovery was exercised directly by the reviewer
  under the guard: seeded committed and prepared/previous leftovers both recovered
  and republished, exit 0.
- **The rewritten claim matches the code.** `README.md:102/112` and
  `README.md:145-153` now state the invariant the check establishes and call it an
  accident guard rather than proof of authorship; the reviewer could not construct
  a protection claim in the changed paragraphs that the code does not support.
- **Residue (a) already covers the dangling-symlink-without-`--replace` case**
  observed by the test author, because it is worded existentially ("a control
  directory left behind by an earlier failed command at that path"). Reproduced by
  the reviewer: that command exits 1 with `publication artifact has type 'link',
  expected file`, seeds the control directory, and a user file placed at the path
  afterwards is then replaced. Naming the symlink explicitly is optional.

Evidence was found sound: 5/5 overlay hashes match HEAD, the worktree and
`6228bcc4`; all six JUnit XMLs parsed independently give unit 174 (3 skipped),
system-focused 105, transport 4, driver 66 (43 unit + 23 system), CLI regression
60 and masked-next 15, with zero failures and errors and fresh run timestamps;
the reviewer re-ran the driver lane (66, exit 0), the focused system lane (105,
exit 0) and the remaining four lanes (250 passed + 3 skipped, exit 0). The fix
touches only `_driver.py`, one test file and the evidence directory.

Four documentation-precision notes are recorded, none of them gating and none a
claim about what the guard protects:

1. `README.md:118` attributes to `replace-guard.log` a ten-item bullet of which
   five items (FIFO, uninitialized control directory, lock-only control directory,
   symlinked control path, control path as a regular file) are recorded in this
   README's round-2 review section instead, and "the publication recovery
   semantics all still work" is not demonstrated in that log either. The reviewer
   re-verified all ten behaviours; only the attribution needs correcting.
2. `README.md:112-116` and `_driver.py:223` say the control directory is
   bootstrapped by "any earlier command that failed at that path". Measured: a
   non-empty `--parameters`, an unpublished unit argument and a missing output
   parent all fail without creating it; only failures reaching the lock set seed
   it. The error is conservative, and the Declared-gaps wording is correctly
   existential.
3. `README.md:145` says "in three ways" while a hand-written marker — reproduced
   in `replace-guard.log` as residue (b) — is a fourth. The strongest-form
   invariant statement and the disclosure that the marker has no secret make it a
   corollary rather than a hidden hole.
4. The heading at `README.md:102` says "naming it" but the emitted diagnostic at
   `README.md:109` / `_driver.py:227` drops the qualifier; for the
   copied-control-directory case one is visibly present with the wrong marker.

Not verified by the reviewer: the 2/2 mutation-check claim (no artifacts); the
shared build against a fresh compile (the C++ is byte-identical to round 1 and
unchanged across rounds 2–3); Windows lock and reparse-point behaviour (POSIX
only, where a dangling junction could still be an instance of residue (a)); and
injected-fault recovery through the driver (committed and prepared leftovers were
constructed directly instead).

### Round-3 disposition

All four notes are applied. F1: `replace-guard.log` now executes the complete
refusal matrix — including the FIFO and the four control-path shapes — and records
for each case whether a control directory existed before and after, so no case
creates one where none existed; the recovery sentence now attributes that exercise
to the reviewer and to the publication package's own suite instead of to that log.
F2: the bootstrap condition is stated as "reached the lock set at that path and
then failed", with the three earlier-failure counterexamples named. F3: the
enumeration now says four and includes the forged marker. F4: the diagnostic is
`refusing to replace a path without a publication control directory naming it:
<path>`, so the copied-control-directory case no longer reads as if no control
directory were present.

The PASS above was returned for `6228bcc4` + `ddbf0449`. F4 is the only note that
touches code, and only the message string: the guard condition, the refusal
behaviour, the exit codes and every case the reviewer verified are unchanged. The
independent test author updated the two assertions that pin the message and the
lanes were re-run afterwards.
