# M4 private per-source compile orchestration (2026-09-29)

Scope: connect the existing components into one usable single-source compile
flow — one Python source snapshot → explicitly supplied managed interface units
→ the existing native source compiler → verified body/header → atomic
publication of a complete source-unit directory. No public CLI, no SDK, no
IR/runtime/manifest change, no first-class `ac.system`.

## Baseline

| Item | Value |
| --- | --- |
| Implementation baseline | `06680be2` |
| Planning baseline | `bd7319ef` (plus the PM's uncommitted archive/ledger edits, left untouched) |
| Shared build | `.pycircuit_out/w10-pm/build` (single integrator) |

## File whitelist and single writer

| File | Writer | Change |
| --- | --- | --- |
| `python/pycircuit/src/pycircuit/_source_compile.py` | this packet (new) | the private orchestration entry |
| `python/pycircuit/src/pycircuit/_source_unit_files.py` | this packet | add a header-only validator next to its existing full validator |
| `compiler/acir/tools/acir-source-unit-harness/acir-source-unit-harness.cpp` | this packet | minimal private `--deps-out` channel plus an explicit owner post-check |
| `tests/unit/test_source_compile.py` | independent test author | unit-level orchestration tests |
| `tests/system/test_source_compile_publication.py` | independent test author | publication/roundtrip tests |

Not modified: ODS and any op/type/attribute schema, the numeric classifier and
the repaired generic/numeric semantics, emitters, runtime, Work/Xfer/reset, the
first-class system / artifact-role / `ac.expect` proposals, public CLI, SDK,
install, `generated.json`, the existing publication transaction semantics,
AGENTS, historical approval records and hashes.

## Lock, snapshot, recovery, and publication design

**One command uses one complete lock set.** A single
`_publication_lock_set(inputs=<every interface unit>, outputs=<the output unit>)`
covers input snapshotting, the native compile, and publication. Inputs take
shared locks, the output takes an exclusive lock, and the helper acquires them
in the existing path order. The forbidden pattern — read each header
individually, release, re-read provider files during compilation, then take a
separate output lock — is structurally impossible here because the native
compiler consumes scratch copies written from the in-lock snapshots and never
re-opens a provider file.

**Owner discovery is never the authority.** `_PublicationInput` requires the
expected owner, so each interface unit's `unit.json` is read strictly (duplicate
keys rejected) *before* locking only to learn `{package, path}`. The lock set
then re-validates that owner against the journal and re-validates the artifact
under the lock, so a substitution between discovery and locking fails closed.

The guard that enforces the last step is `_publication.py:191` (`published input
owner does not match`). That claim is a code-level argument here, not a repository
test: the message appears nowhere under `tests/`, so this packet does **not** claim
committed coverage of the swap race. It was exercised only by the reviewer's own
adversarial provider-swap script, which published nothing. A permanent regression
test belongs to an independent test author, not to this packet.

**Normal reads and recovery validation are separate**, using the two validators
`_PublicationInput` already distinguishes:

- `stable_validate` = the new header-only validator (receipt + interface), so a
  parent compile accepts a header-only provider and never reads a child body or
  depfile as a semantic input;
- `recovery_validate` = the existing `_validate_full_source_unit`, so an
  unfinished transaction, or a committed-but-cleanup-pending state, is checked
  against the complete artifact before anything is consumed;
- the output's `recovery_validate` is the same full validator, and
  `_publish_artifact` re-validates the published tree.

A normal parent compile is therefore never forced to read child bodies, and
recovery is never skipped to make header-only consumption work.

**Semantic versus file validation.** Publication only happens after the native
helper returned success, which already runs the MLIR verifiers on both artifacts
and (new) checks that `ac.source_owner` on body and interface equals the
requested owner. Python performs no MLIR parsing, no import guessing and no
type/ownership analysis; `_validate_full_source_unit` remains a file/receipt
check, not a semantic one.

## Depfile

The helper previously reported nothing about what it consumed, so a depfile
written from the supplied `--header` arguments would have listed unconsumed
inputs and hidden the omission. Measured facts:

- `SourceHeaderRegistry` exposes `suppliedHeaders()` but no consumed set;
- the compiled body records `ac.interfaces`, and measurement shows it lists the
  **consumed interface closure**: supplying an extra unrelated header does not
  add it.

So the minimal private channel is `--deps-out <path>`: after a successful compile
the helper writes a strict JSON array of `{"package","path"}` for `ac.interfaces`
minus the unit's own owner, sorted. The depfile is then built from verified
consumption:

- target: the **published** `<stem>.ac` (final path, never the staging path);
- prerequisites: the original source path, each consumed interface unit's
  `<stem>.interface.ac` **and** `unit.json`, and the native helper path itself;
- scratch transport, scratch header snapshots, helper bodies and the helper
  response are never prerequisites;
- escaping follows Make: `\` → `\\`, space → `\ `, `#` → `\#`, `$` → `$$`;
- there is no `-I`-style implicit search path in this flow: only explicitly
  supplied and actually consumed interface units become prerequisites, and any
  consumed owner that was not supplied fails closed.


## Entry signature and usage

```python
from pycircuit._source_compile import _compile_source_unit

result = _compile_source_unit(
    "src/counter.py",              # the one source being compiled
    source_root="src",             # capture confinement root
    package="demo",                # source-unit package prefix
    native_compiler="/path/to/acir-source-unit-harness",
    output="units/counter",        # published unit directory
    interface_units=["units/types"],   # explicit managed interface units
    replace=False,                 # same-owner replacement
    cancelled=None,                # optional publication cancellation hook
)
```

Returns `_SourceCompileResult` with `destination`, `owner`, the published
`body`/`interface`/`depfile` names and the `_PublicationResult`. The output
directory's parent must exist and no path component may be a symlink, matching
the rest of the publication package.

## Verification

The environment exported for every lane is recorded in `lane-env.txt`:
`ACIR_SOURCE_UNIT_HARNESS` and `ACIR_DESIGN_HARNESS` (the shared build's
helpers), `MLIR_OPT` and `PYTHONPATH`.

| Lane | Log | Tests | failures | errors | skipped | exit |
| --- | --- | --- | --- | --- | --- | --- |
| unit | `unit.xml` | 174 | 0 | 0 | 3 | 0 |
| system focused | `system-focused.xml` | 105 | 0 | 0 | 0 | 0 |
| LLVM transport | `transport-mlir.xml` | 4 | 0 | 0 | 0 | 0 |

Selector inventory of the system lane: `test_source_compile_publication` 6,
`test_source_unit_packet` 51, `test_generic_assignment_roundtrip` 7,
`test_source_design_bridge` 41. All three lanes exit 0.

The skipped unit cases are the pre-existing platform skips in the publication
filesystem suite, unchanged by this packet.

### Pre-existing failures, reported not hidden

`tests/system/test_source_module_units.py` collects **22** tests. With the
harness configured, **17 fail and 5 pass**, and every failure carries the
frontend's by-design rejection of that file's class/self authoring fixtures:
`@module and @system require function definitions; class/self authoring has been
retired`. Measured against the **baseline tree** (`06680be2` in a detached
worktree; same harness binary, same environment, only the checkout differs) the
result is identical: baseline **17 failed, 9 passed**, candidate **17 failed,
9 passed**, and the two failure sets are byte-identical after sorting
(`stale-tests-baseline.log`, `stale-tests-candidate.log`). It is therefore a
pre-existing stale fixture, not a regression from this packet, and it is
excluded from this packet's lanes rather than repaired here — repairing it is a
separate bounded task. (Without `ACIR_SOURCE_UNIT_HARNESS` the same file fails
earlier, in all 22 tests, with `set ACIR_SOURCE_UNIT_HARNESS for U02-A system
tests`; that guard is environmental and is not the failure mode reported here.)

`tests/system/test_source_transport_mlir.py` is **not** a stale fixture and is
**not** excluded. Its **4** tests are gated on a configured `mlir-opt`, which is
keg-only in this environment and absent from `PATH`. With `MLIR_OPT` set the file
is **4 passed** on both the baseline tree and the candidate
(`stale-tests-candidate.log`); with `MLIR_OPT` unset all 4 fail at
`tests/system/test_source_transport_mlir.py:27` with `LLVM 22 mlir-opt is
required; set MLIR_OPT or PYC_TOOLCHAIN_ROOT`
(`transport-mlir-unconfigured.log`). The file imports only the pure-Python
capture and transport modules, neither of which this packet modifies, so it is
reported as its own lane above instead of being counted among "stale" failures.

This subsection was corrected after independent review. The earlier revision
reported collection counts of 15 and 6 for these two files and attributed all
four transport failures to retired class/self fixtures; the measured counts,
the two distinct failure modes, and the separate `MLIR_OPT` gate are above. The
earlier combined unconfigured run (`baseline-stale-tests.log`, "21 failed, 5
passed") is superseded by the three logs named here.

### What the tests prove

- **Declaration unit**: a type-only source publishes the closed four-file set
  with `ac.unit_kind = "declarations"` and no fabricated `ac.module`/`ac.system`.
- **Header-only parent**: after the provider's Python source, body, and depfile
  are deleted, the parent still compiles from its managed interface; the depfile
  names the provider's `interface.ac` and `unit.json` and never its body.
- **One source per invocation**: the fake compiler logs its `--path`, and one
  call yields exactly one invocation for exactly that source.
- **Depfile**: the target is the published `<stem>.ac`; prerequisites are the
  original source, the consumed provider's interface plus receipt, and the native
  compiler; a supplied-but-unconsumed provider and every scratch input are
  absent; `src dir#1$` is escaped as `src\ dir\#1$$`.
- **Replace**: the same owner republishes all four files; a native failure leaves
  the previous four files byte-identical.
- **Fail-closed**: unmanaged interface directories, two units declaring one
  source, a consumed-but-unsupplied dependency, a native failure, and a compiler
  that produces no artifacts are all rejected without publishing.
- **Bridge**: the real artifacts produced by the entry drive the existing design
  harness (link → `--target cpp`/`verilog` → run), and the design executes on both
  backends with traces `[254, 3, 3]` and reset/rerun equality.

## Limits

- This is a private callable entry. It does not register anything on the public
  `pycircuit` command, does not install, and does not switch the old route.
- Only direct-current and literal generic assignments are exercised; no new
  source capability was added.
- The `--deps-out` channel is a private helper output, not an ACIR attribute or a
  public protocol; it reports the consumed interface closure the helper already
  computed.
- The publication transaction semantics, receipt schema, `unit.json`, owner
  kinds and recovery protocol are unchanged.
- This packet is M4's per-source compile step. It does not complete M4, the
  public CLI, first-class system, SDK, or M1-M7.

## Independent review

Requested from a separate instance; not self-signed.

**First review, of `64cb6106`: FAIL — evidence integrity only.** The reviewer
confirmed the functional claims and the interface behaviour, and rejected the
packet on the evidence in this directory. Two HIGH defects and one LOW were
raised; all three are fixed here.

| # | Finding | Disposition |
| --- | --- | --- |
| 1 | HIGH — this README reported collection counts of `15` and `6` for the two environment-gated files. | Corrected to the measured `22` collected (`17` failed, `5` passed) and `4` collected; see "Pre-existing failures, reported not hidden". |
| 2 | HIGH — the four `test_source_transport_mlir.py` failures were attributed to retired class/self fixtures. They are a missing `mlir-opt` in `PATH`, not a stale fixture. | Re-measured with `MLIR_OPT` exported: `4 passed`, identical on the baseline tree and the candidate. The file is now its own lane, and the unconfigured run is kept as the tool-gate negative control (`transport-mlir-unconfigured.log`). |
| 3 | LOW — `_source_compile.py` re-read the interface unit receipt inside the held lock, a redundant second authority for a value the lock set had already validated. | Replaced with a reuse of the owner captured before locking (`51ec9ab5`); `_PublicationInput` remains the only authority, and the lock set still re-validates the receipt under the lock. |

**Re-verification** was requested for `51ec9ab5` plus the evidence commit
carrying that table, and the round-2 verdict is recorded below.

## Independent review — re-verification (round 2), verdict PASS

**Verdict: PASS** for `51ec9ab5` (code) + `d6233d1e` (evidence). The round-1 FAIL
was evidence-integrity only; both HIGH evidence defects and the LOW code item are
corrected, and the functional claims still hold on the new revision.

**Lanes re-measured by the reviewer, independently of the packet's logs.** Parsed
the raw XML: unit **174 / 0 / 0 / 3**, system focused **105 / 0 / 0 / 0**
(selectors 6+51+7+41), LLVM transport **4 / 0 / 0 / 0**. Re-ran all three lanes
here: `171 passed, 3 skipped` exit 0; `105 passed` exit 0; `4 passed` exit 0. The 3
skips are the pre-existing Windows-only cases in `tests/unit/test_publication_fs.py`.
XML timestamps (00:43:51, 00:43:56, 00:44:26 +08:00) all post-date the `51ec9ab5`
commit (00:43:47), and code is byte-identical between `51ec9ab5` and `d6233d1e`.

**Finding dispositions.**
1. **HIGH, counts — fixed.** `test_source_module_units.py` collects 22 (17 failed /
   5 passed with the harness configured), not 15; `test_source_transport_mlir.py`
   collects 4, not 6.
2. **HIGH, attribution — fixed.** `mlir-opt` is keg-only and absent from `PATH`;
   with `MLIR_OPT=/opt/homebrew/Cellar/llvm/22.1.8/bin/mlir-opt` the file is 4
   passed on both trees; unset, all 4 fail at
   `tests/system/test_source_transport_mlir.py:27` with the tool-gate message.
   Negative control and its own lane are recorded.
3. **LOW, redundant in-lock receipt read — fixed and verified.** An instrumented
   run shows the owner handed to the in-lock snapshot is *the same object* the lock
   set validated (single authority); substitution between discovery and locking
   still fails closed and publishes nothing.

**Baseline comparison, redone.** Fresh `git worktree add --detach … 06680be2`: all
**6461/6461** tracked files byte-identical to the commit. Same harness, same env,
`MLIR_OPT` set → baseline **17 failed / 9 passed**, candidate **17 failed / 9
passed**, sorted failing-ID sets **identical**; per file, module_units 17 failures
on both, transport 0 failures on both. *Environment note:* the common config
carries `core.worktree`, so in a fresh worktree `git status`/`git diff` resolve
against `/Users/zhoubot/linx-isa/tools/pyCircuit` — the checkout bytes are correct
(hash-verified). Temporary worktree removed.

**Hash provenance.** All five `overlay-sha256.txt` entries match both
`git show HEAD:<path> | shasum -a 256` and the working-tree bytes.

**Functional claims 1–9 and 11 — still confirmed at `51ec9ab5`.** Adversarial
scripts re-run: claims 1–3 18/18, claim 7 20/20, claims 4/5/6/11 confirmed
(header-only provider accepted while the full validator rejects it; depfile
target/prereqs correct with a supplied-but-unconsumed provider absent; `--deps-out`
= consumed closure only; Python imports only stdlib + private modules — no MLIR
parsing, no import guessing; publication gated on helper exit status). Scope since
round 1 is only `_source_compile.py` (6+/2−) plus evidence docs — no ODS/`.td`,
emitter, runtime, CLI, SDK, manifest, first-class system/artifact-role/`ac.expect`,
or approval record; `_publication.py`/`_publication_fs.py` untouched.

**Non-blocking note.** Ledger `e903fe7c`: the sentence following the module_units
measurement reads "…复跑得到同样的 17 failed / 9 passed" — 17/9 is the combined
two-file result (per file it is 17 failed / 5 passed and 4 passed). Numbers are
right for the run described; optional clarifying edit.

### Correction to the reviewer's round-1 report (quoted)

> My script's lone "entry does not import an MLIR parser" FAIL was my own
> over-broad substring grep hitting the docstring's "native MLIR compiler". The
> proper AST audit shows only `__future__`, `collections.abc`, `functools`,
> `json`, `pathlib`, `subprocess`, `tempfile` and the five private `._*` modules.

### Reviewer's stated limits (quoted)

> No project rebuild (round 1 linked a genuine baseline harness read-only from
> existing archives; round 2 used the shared candidate harness for both trees,
> sound because the helper delta is additive and the stale files never pass
> `--deps-out`); fault-injection matrices, `cancelled` hook and Windows-only paths
> rest on the packet's passing suites; no independent MLIR semantic adjudication
> beyond the real harness.

### Reviewer's evidence (quoted)

> - `git diff 51ec9ab5 d6233d1e -- python/ compiler/ tests/` → empty.
> - Exact task command → **17 passed**, exit 0; unit lane **171 passed/3 skipped**
>   exit 0; system lane **105 passed** exit 0; transport with `MLIR_OPT`
>   **4 passed** exit 0.
> - XML parse → 174/0/0/3, 105/0/0/0, 4/0/0/0; overlay hashes all match committed
>   bytes.
> - Baseline vs candidate: **17 failed/9 passed both, IDENTICAL FAILURE SETS**;
>   6461/6461 files hash-match `06680be2`.
> - Unconfigured controls: module_units **22 failed** (harness guard), transport
>   **4 failed** (line-27 mlir-opt gate).
> - `verify_claims_123.py` 18/18, `verify_claim7.py` 20/20,
>   `verify_owner_reuse.py` PASS; `git worktree list` no longer shows the
>   reviewer's worktree; repo clean.
