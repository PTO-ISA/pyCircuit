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

| Lane | Tests | failures | errors | skipped |
| --- | --- | --- | --- | --- |
| unit (`unit.xml`) | 174 | 0 | 0 | 3 |
| system focused (`system-focused.xml`) | 105 | 0 | 0 | 0 |

Selector inventory of the system lane: `test_source_compile_publication` 6,
`test_source_unit_packet` 51, `test_generic_assignment_roundtrip` 7,
`test_source_design_bridge` 41. Both exit codes are 0.

The skipped unit cases are the pre-existing platform skips in the publication
filesystem suite, unchanged by this packet.

### Pre-existing failures, reported not hidden

`tests/system/test_source_module_units.py` (15) and
`tests/system/test_source_transport_mlir.py` (6) contain class/self authoring
fixtures that the current frontend rejects by design
(`@module and @system require function definitions; class/self authoring has been
retired`). Running those two files against the **baseline** harness
(`git stash` of this packet's helper change, rebuilt) gives **21 failed, 5
passed** — exactly the same as with the packet applied
(`baseline-stale-tests.log`). They are therefore pre-existing stale fixtures, not
a regression from this packet, and they are excluded from this packet's lanes
rather than repaired here (repairing them is a separate bounded task).

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

Requested from a separate instance; not self-signed. The verdict is to be
appended before the packet is accepted.
