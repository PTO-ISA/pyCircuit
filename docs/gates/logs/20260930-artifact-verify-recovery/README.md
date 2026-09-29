# C3 publication-artifact verification and managed input recovery (2026-09-30)

Scope: close three reproduced defects in the C3 `compile`/`link` driver — an
existing linked artifact was replaced without validating its IR or owner, a
replaced source unit was validated only at the file level, and a managed input in
a legitimate unfinished transaction was reported as missing instead of being
recovered. The public `compile`/`link` surface and semantics are unchanged.

## Baseline

| Item | Value |
| --- | --- |
| Implementation baseline | `0f4dedb1` |
| Planning baseline | `49b73c8c` |
| Shared build | `.pycircuit_out/w10-pm/build` (single integrator) |

## Frozen private native calling convention

Two read-only verification shapes are added to the private
`acir-design-harness`. They emit no hardware artifact, take no lock, and reuse
the existing `--entry-owner-out` JSON shape, the existing `jsonString` escaper and
the shared native verifier (`mlir::verify` plus
`buildFinalProgramFromHardware` / `lowerExactInputAddTransactional`, the same
calls `runLink` and `runEmit` already make). They are private helper modes, not a
public CLI, IR or receipt protocol, and they change no schema.

### Program verification

```text
acir-design-harness --design <program.ac> --verify-only [--entry-owner-out <path>]
```

- parses `program.ac`, runs `mlir::verify`, then reconstructs the final program
  with the same call the emit path uses, so "valid" means the shared verifier
  accepts it, not that Python guessed;
- exit 0 only when both succeed; a parse failure, a verifier failure or a root
  without `ac.source_owner` exits non-zero with a diagnostic and writes no report;
- `--entry-owner-out` writes exactly the existing shape, unchanged:

```json
{"source":{"package":"<str>","path":"<str>"},"definition":"@\"<canonical>\""}
```

- forbidden in this mode: `--body`, `--header`, `--top`, `--target`, `--output`,
  `--glue-output`, `--role`, `--unit-owner-out`.

### Source-unit verification

```text
acir-design-harness --body <stem>.ac --header <stem>.interface.ac --verify-only \
  [--unit-owner-out <path>]
```

- parses the body and the interface, applies `lowerExactInputAddTransactional` to
  an `implementation` body exactly as `runLink` does, then runs `mlir::verify` on
  both;
- exit 0 only when both parse and verify; otherwise non-zero, no report;
- `--unit-owner-out` writes one closed object with both internal owners, so Python
  can compare each against the receipt and the expected publication owner:

```json
{"body":{"package":"<str>","path":"<str>"},"header":{"package":"<str>","path":"<str>"}}
```

- exactly one `--body` and one `--header` are required; `--design`,
  `--top`, `--target`, `--output`, `--glue-output`, `--role` and
  `--entry-owner-out` are forbidden.

Both shapes are resolved through the existing `ACIR_DESIGN_HARNESS` override or
the bundled toolchain, like every other private helper call.

## File whitelist and single writer

| File | Writer | Change |
| --- | --- | --- |
| `compiler/acir/tools/acir-design-harness/acir-design-harness.cpp` | this packet | the two read-only verify shapes, reusing the existing JSON writer |
| `compiler/acir/lib/Dialect/ACIR/ACIRHardwareClosure.cpp` | this packet | one-word `dyn_cast_or_null` crash fix, see below |
| `python/pycircuit/src/pycircuit/_native_verify.py` | this packet (new) | helper resolution, invocation, closed-report validation |
| `python/pycircuit/src/pycircuit/_source_unit_files.py` | this packet | `_discover_source_unit_owner` reads the receipt or the validated journal; `_read_full_source_unit` gains native verification and internal-owner comparison |
| `python/pycircuit/src/pycircuit/_driver.py` | this packet | managed-aware preflight, native program validator, private `filesystem=` on `link_command` so the real fault hooks can be driven |
| tests | independent test author | failing-first regression per the packet prompt |
| `docs/gates/logs/20260930-artifact-verify-recovery/` | this packet | evidence |

`ACIRHardwareClosure.cpp` was not in the expected whitelist and is added
deliberately. `hardware_detail::specKey` used `dyn_cast<DictionaryAttr>(raw)`, and
LLVM's `dyn_cast` requires a non-null value, so a package with no `ac.entry` — the
`module {}` shape, or a unit body handed to `--design` — dereferenced a null
attribute and died with SIGSEGV before any diagnostic. The verify mode inherits
that path, so without this fix the packet's own claim that a verifier failure
exits non-zero with a diagnostic would be false, and the pre-existing `emit` path
has the same crash on the same input. One word, `dyn_cast_or_null`, makes both
fail closed with `final program rehydration requires verified final hardware`.
The 20-test native `ACIR*` ctest suite was re-run for this shared-verifier change
and passes.

`_source_compile.py` is **not** modified: its `-I` discovery already goes through
`_discover_source_unit_owner`, and its preflight through the driver's
`_require_published_unit`, so both inherit the managed-state handling without a
change. Its header-only `stable_validate` is deliberately untouched.

Not modified: the public `compile`/`link` command shapes, public `emit`, ODS and
any IR schema, runtime, SDK, packaging, `generated.json`, the receipt/journal/
manifest field sets, the two unapproved revision-B proposals, the numeric
classifier, accepted hardware timing, the Windows lock implementation, and the M5
cutover.

## Defects and intended fixes

**A. An existing linked artifact must be IR- and owner-verified.**
`_validate_published_program` currently checks only that the target is a regular
non-empty file and ignores the `owner` argument, so a foreign-owner artifact, a
plain user file, or a corrupt one is replaced. It becomes: run the program verify
shape, extract the actual `SourceOwner` and definition from the verified root,
and require an exact match with the owner the publication callback passed in.
Corrupt, non-IR, non-final or wrong-stage artifacts are rejected with the original
bytes untouched. The control marker stays a path guard only and is never treated
as proof of validity.

**B. A replaced source unit must be IR-verified too.**
`_validate_full_source_unit` checks the closed file set, the receipt, file types
and text readability. It gains: native verification of body and header, and a
comparison of both internal owners against the receipt owner and the expected
publication owner. It validates the *old* artifact's own consistency, so a unit
built against older dependencies is still replaceable — being stale is not being
corrupt. Header-only consumption for a stable provider is unchanged: only the
full validator is strengthened, never the header-only one.

**C. Inputs must be recovered before a target is judged missing.**
`_require_published_unit` and `_discover_source_unit_owner` read the destination
directory or receipt before the lock set can recover it, so a legitimate
`journal.phase = prepared`, `destination` absent, `previous` and `stage` present
state is reported as a missing input. Owner discovery becomes managed-state first:
a stable managed unit is read from its receipt as today; otherwise the validated
journal owner is used, and `_publication_lock_set` — which already recovers a
non-committed input before yielding, using `journal["owner"]` — performs the
recovery, after which the recovered artifact is re-validated against that same
owner. No lock is upgraded, no post-recovery read is mixed with a pre-recovery
one, and a corrupt control state or journal is refused rather than falling back to
unmanaged guessing. Both the public `compile -I` path and `link` use it.

## Verification

Environment in `lane-env.txt`. Counts were read by parsing the raw JUnit XML, not
the pytest summary line. Every lane exits 0.

| Lane | Log | Tests | failures | errors | skipped |
| --- | --- | --- | --- | --- | --- |
| unit core (previous packets) | `unit-core.xml` | 175 | 0 | 0 | 3 |
| driver unit | `driver-unit.xml` | 48 | 0 | 0 | 0 |
| system focused (previous packets) | `system-focused.xml` | 105 | 0 | 0 | 0 |
| driver system + new regression | `driver-system.xml` | 47 | 0 | 0 | 0 |
| LLVM transport | `transport-mlir.xml` | 4 | 0 | 0 | 0 |
| CLI regression | `cli-regression.xml` | 60 | 0 | 0 | 0 |
| masked-next | `masked-next.xml` | 15 | 0 | 0 | 0 |
| native `ACIR*` ctest | `native-acir-ctest.log` | 20 | 0 | 0 | 0 |

**The three skips are named, not hidden**, and they are the pre-existing Windows
cases in `tests/unit/test_publication_fs.py`: `requires real Windows filesystem
APIs`, `requires Windows reparse-point support`, and `requires real Windows
directory handles`. No lane in this packet is described as zero-skip.

### Failing-first evidence

The independent test author's two files were run against a pristine `0f4dedb1`
export with the same built helpers (`baseline-failing-first.log`): **18 failed /
10 passed**, and every failure is the defect this packet closes — eleven
corruption/owner cases failed because `--replace` *succeeded* (`AssertionError:
(0, '')`) or, for the non-UTF-8 binary target, because the refusal was attributed
to the protocol's generic wrapper instead of the corruption family; the
prepared-transaction cases failed with `compile interface unit is not a
directory` / `link unit is not a directory`; and owner discovery raised
`source-unit receipt is not stable UTF-8` instead of reading the validated
journal. The same files are 28 passed on this candidate. The three unit files the
author also updated are coupled to the new surface by construction and are
therefore green only here; they are regression maintenance, not failing-first
evidence.

Two further points the author established by running, not by argument: the
rollback points `after_journal_prepared`, `after_stage_complete` and
`after_journal_committed` already recovered on the baseline because they leave the
destination present, so only the `after_previous_saved` discovery path was
broken; and the `documentation_layout` / `repository_layout` / `primitive_catalog`
/ `pyc_ir_inventory` unit failures are pre-existing and unrelated — the layout
check explicitly skips `docs/gates/logs/`, and the extra items belong to other
workstreams.

## Implementer smoke verification

`verify-smoke.log` records the operations end to end on this candidate: both
native verify shapes with their reports; a foreign-owner program replacement
refused with `published program owner does not match` and the bytes unchanged; a
non-IR artifact refused with a one-line `failed native verification` diagnostic
and the bytes unchanged; a legitimate same-owner republish succeeding; a
corrupted unit body refused by `compile --replace` with the corrupted bytes left
in place; and both interrupted transactions — source unit and program — forming
`phase: prepared`, destination absent, `previous` and `stage` present, then being
recovered by the public `compile -I` and `link` with the journal cleared.

## What this packet does not claim

It proves that an existing artifact satisfies the IR contract, that its internal
owner agrees with the request or the journal, that corrupt, foreign-owner and
ordinary user files are not overwritten, and that a legitimate transaction is
recovered by the existing state machine. It does **not** provide cryptographic
authentication of the historical author, and it cannot distinguish a legitimate
same-owner hand edit of final IR from the artifact this driver published. It
distinguishes syntax/semantic corruption, owner mismatch, and missing recovery
evidence — nothing more.

It also does **not** validate a depfile's contents beyond stable UTF-8 and
non-emptiness, so a garbage depfile in an otherwise valid unit is accepted, and it
cannot tell whether a published artifact was produced here or copied in from
elsewhere with matching owners.

## Commits

| Commit | Content |
| --- | --- |
| `bb2b7bf3` | the three fixes, the private verify shapes, the shared-verifier crash fix, and all five test files |
| `1ed84f1a` | the round-4 residual fix: an undecodable program reports in the corruption family, with its regression case |
| this evidence commit | the round-4 review record, the corrected failing-first count and the recomputed lanes |

Overlay hashes for the ten changed files are in `overlay-sha256.txt`, computed
from the committed bytes.

## Independent review record (round 4)

Reviewer: independent agent, not the author. Reviewed `bb2b7bf3` and `60f128be`
at HEAD `60f128be` in `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Baseline `0f4dedb1` obtained with `git archive` (no worktree added). Shared build
verified current with HEAD (`ninja -n acir-design-harness acir-source-unit-harness`
→ no work to do). Checkout left clean; scratch in /tmp only.

**Verdict: PASS.** All eight claims re-derived from code and raw artifacts with
the reviewer's own reproducers, not the packet's tests.

- **A/B/C reproduce and are closed.** On `0f4dedb1`: foreign-root relink, plain
  text, syntax-corrupt, non-final-stage and foreign-owner targets all exit 0 and
  overwrite; a corrupt body/interface/owner unit exits 0 and is replaced; public
  `compile -I` and `link` on a `phase = prepared` provider (destination absent,
  `previous` + `stage` present) exit 1 with `is not a directory` and leave the
  journal in place. On HEAD every corruption/owner case exits 1 with one
  diagnostic line and byte-identical artifacts and control directories, and both
  public commands recover the transaction, restoring the destination
  byte-for-byte and clearing the journal. Positive controls (legitimate
  same-owner replace) still exit 0.
- **Reasons stay distinguishable**, one family each: `failed native verification`,
  `source-unit internal owner does not match its receipt`, `source-unit receipt
  owner does not match`, `published program owner does not match`, `without a
  publication control directory naming it`, `source-unit destination is missing
  or is not a directory`, `publication journal is not strict JSON`. Baseline
  collapsed the last two into `compile interface unit is not a directory`.
- **No new public surface.** `pycircuit --help` / `compile --help` / `link --help`
  byte-identical to baseline; no `.td`, `generated.json`, schema, SDK, runtime or
  `__init__` change; `--verify-only` appears only in the private
  `_native_verify.py`. The only signature change is the disclosed private
  `filesystem=` keyword on `link_command`.
- **Callback boundary holds.** `_publication.py` is byte-unchanged. Replacement
  passes the request's owner (lines 388/400/417); recovery passes
  `journal["owner"]` (911/971/979). Both new validators use the path they are
  given — proven by a fresh publication validating `<control>/stage` with no
  destination yet existing. A cross-owner replacement is refused before any
  journal is written, and a committed-but-corrupt target is reported with the
  journal, `previous`, and the corrupt bytes retained, never silently rolled back.
- **Units are validated as themselves.** A dependent whose snapshot provably
  predates the current provider is still replaceable (stale ≠ corrupt); a
  provider whose body is deleted still serves `compile -I` while `link` refuses
  it.
- **Crash fix is one word and bounded.** Reconstructed baseline-`specKey` harness
  segfaults (rc=139) on `module {}` and `module attributes {ac.stage = "final"} {}`
  through both the new verify shape and the pre-existing emit path; HEAD exits 1
  with a diagnostic and no report. Valid programs still verify (rc=0).
- **Evidence integrity.** All ten overlay hashes match `git show HEAD:<path>` and
  the working tree. Raw JUnit re-parsed: unit-core 175 (3 named Windows skips),
  driver-unit 48, system-focused 105, driver-system 46, transport 4,
  cli-regression 60, masked-next 15, native ctest 20/20. The failing-first log was
  independently reproduced against a pristine `0f4dedb1` export: 17 failed /
  10 passed, same node IDs and messages. The TableGen error was reproduced
  directly with `mlir-tblgen` from byte-identical `.td` files at both revisions,
  and the four unrelated unit failures fail identically at baseline.
- **Limits are honest.** A same-owner hand edit of published final IR is accepted
  by `link --replace`, exactly as the packet states.

Non-blocking residuals, recorded for completeness, not as defects: a non-UTF-8
binary target is refused through the pre-existing generic wrapper
`published artifact validation failed: <path>` (identical on baseline, still fails
closed); the depfile is only checked for stable UTF-8 and non-emptiness, so a
garbage depfile is accepted; `link_command` gains a disclosed private
`filesystem=` keyword. No other deviation found.

### Round-4 disposition

The PASS above was returned for `bb2b7bf3` + `60f128be`. Residual one was fixed
rather than merely recorded, because the packet's own contract says corruption is
its own refusal family and a non-UTF-8 artifact is corruption:
`_validate_published_program` now decodes explicitly and raises
`published program is not UTF-8 text` on a decode failure, so the generic
`published artifact validation failed` wrapper no longer appears for this case.
Observed: a program overwritten with `\x00\x01\xff\xfe\x00binary` plus
`link --replace` exits 1 with that one line and the bytes unchanged. Residual two
is now enumerated under "What this packet does not claim" above. Residual three
was already disclosed. The guard behaviour, the callback boundary and every other
diagnostic are unchanged; the independent test author added a regression case for
the new branch.

One number in the review record above is superseded. The reviewer measured the
failing-first run before that extra case existed, so it says 17 failed / 10
passed; with the new case included the run is **18 failed / 10 passed**, the
eighteenth failing for exactly the wrapper-attribution reason the reviewer
flagged. The lane table above carries the current driver-system count (47).
