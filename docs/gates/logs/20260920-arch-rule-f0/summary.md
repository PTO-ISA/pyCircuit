# Architecture-rule F0 decision convergence

Date: 2026-09-20

Scope: documentation and decision convergence only. This evidence does not
claim implementation or verification of F1 or any later phase.

## Result

- Registered accepted Decisions 0271–0274 for exact persisted rule-effect
  DAGs and shared effect-graph analysis, first-class safety-only architecture
  obligations, exact four-state/SRAM verification, and the one-stage
  pointer-owned composition/naming hard break.
- Recorded Decisions 0269 and 0270 as implemented-verified using the existing
  composite-module closure evidence. Decision 0269 is historical/superseded
  closure; Decision 0270 is the active implemented contract.
- Recorded Decisions 0271–0274 as `gap-in-scope`. No compiler, runtime,
  backend, or test implementation is claimed by this F0 change.
- Preserved Decision 0265 unchanged.
- Reconciled the four architecture documents and made all three supplemental
  RFC/checklist documents reachable from MkDocs navigation.

## Baseline and commands

- `git status --short --branch` showed branch
  `codex/acc-naming-contract...origin/codex/acc-naming-contract`, pre-existing
  edits to `docs/rfcs/architecture-rule-compiler-extension.md` and `mkdocs.yml`,
  and the three supplemental RFC/checklist documents as untracked user-owned
  work. Those documents were preserved and converged in place.
- `rg -n '^## Decision 027[0-9]' docs/rfcs/pyc6-decisions.md` showed 0270 as the
  highest concurrent decision before this slice; 0271–0274 were free.
- `docs/gates/logs/20260919-acc-composite-modules/summary.md` reports focused
  frontend/CLI 58 passed, composite package lit 2 passed, CodeGenTests 109
  passed, CompilerTests 9 passed, GfsimTests 261 passed, plus repository,
  diagnostic, API, MkDocs, and diff checks. This is the historical evidence
  used for 0269/0270 only.
- pyCircuit baseline: branch `codex/acc-naming-contract`, HEAD
  `9fbe66baf2b8c44918702ac85647e22416b82afe`, upstream
  `origin/codex/acc-naming-contract`, ahead/behind `0/0`, target `origin/main`,
  and no PR for the current branch. Remotes were `origin` (PTO-ISA/pyCircuit),
  `agentic-circuit` (PTO-ISA/agentic-circuit), `linx-fork`
  (LinxISA/pyCircuit), `myfork` (zhoubot/pyCircuit-1), `hengliao`, and `hmljy`.
  The dirty files listed above are user-owned and were preserved.
- SuperScalarModel baseline: branch `feat_pyc_model`, HEAD
  `8b640c0f81f9f975480a6d7767c5df1e621172a3`, upstream
  `origin/feat_pyc_model`, ahead/behind `0/0`, target `feat_pyc_model`; the
  sole remote was `origin` (`LinxISA/SuperScalarModel`), which did not resolve through the PR
  query, so current-branch PR status is unavailable. Its existing modified,
  untracked test, document, and probe files are consumer-owned and untouched.
- SSM's authoritative `toolchains/pycircuit.lock.toml` pins pyCircuit
  `9fbe66baf2b8c44918702ac85647e22416b82afe` with `runtime_abi=false`.
  `docs/model/status/profile-matrix.toml` is stale at `831ad1f...`, while
  `docs/model/inventory.json` retains the unavailable dirty catalog snapshot
  `b81ecfc...`; those profile/plan pins are not current closure evidence.
- SSM inventory/layout baseline: `check_pyc_source_layout.py` reported 18
  multi-public-module violations. `check_pyc_module_decls.py` reported 20
  declaration sources, 12 managed H1/H2 sources, and one pending implementation
  (`bctrl`). These are baseline gaps, not F0 framework failures.
- `git diff --check` passed.
- `python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md`
  passed: `ok: API hygiene check passed`.
- `mkdocs build --strict` passed: documentation built successfully.
- `python3 flows/tools/check_decision_status.py --status docs/gates/decision_status_v6.md --out /tmp/pycircuit-f0-decision-status-coverage.json`
  parsed complete decision coverage and exited nonzero exactly because
  `0271 0272 0273 0274` are unresolved in-scope gaps.
- `python3 flows/tools/check_decision_status.py --status docs/gates/decision_status_v6.md --out /tmp/pycircuit-f0-decision-status-strict.json --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence`
  exited nonzero with exact results:
  `unresolved in-scope decision gaps: 0271 0272 0273 0274` and
  `non-verified decisions remain: 0265 0271 0272 0273 0274`.
  This is the expected fail-closed F0 state, not an F0 test failure.

## Architecture verdict applied

- The current serialized rule summary is incomplete because it drops exact
  normalized index/predicate expressions. F1 may change that representation;
  F2 must reuse existing value-constraint and writer-arbitration proofs.
- `ac.marker.obligation` remains a transient handshake marker.
  `ac.arch_obligation` is a distinct first-class safety contract with explicit
  typed sampling, closure, and runtime-admission rules.
- Runtime assertions cannot legalize overlap or one-hot optimization, cannot
  replace synthesis/deployment proof, and cannot be disabled when admission
  depends on them.
- Same-cycle persistent-state reads keep committed old-state observation unless
  an explicit typed forwarding relation is separately accepted.
- Four-state parity requires exact known/Z masks and equal values on known bits.
  Aggressive SRAM verification includes `N=1` and an edge-based one-live-cycle
  Q lifetime.
- Pointer ownership and readable C++/RTL naming migrate in one hard cutover:
  unique child ownership, parent Queue values, typed raw Queue pointers, wide
  immutable shared payloads, source-owned `.hpp`/`.cpp` stems and nominal type
  shards, explicit typed parameters, and no dual/fallback naming or emitter.
- Arbitration selects the winner before preparation; losing candidates reserve
  and publish nothing.

## Remaining closure

- Decision 0265 still requires real Windows candidate/stable runner evidence.
- Decisions 0271–0274 require their documented F1/F3/F6/F4–F5 implementation
  and verification work before strict release closure can pass.

## Independent review

- Architecture pre-review: PASS after the six F0 policies and the pointer/naming
  hard-break boundary were frozen in Decisions 0271–0274.
- Code review: PASS after reconciling Decisions 0264/0269/0270/0274, closing the
  sampling tagged union and severity enum, and updating the hard-break ledger.
- Evidence verification: PASS. The two-repository baseline, contributor routing,
  P0 checklist, command manifest, durable ledger copy, and expected decision-status
  blockers are all present and internally consistent.

## Focused correction

The final F0 review tightened the accepted contracts without changing status:

- exact expression DAGs now define closed opcodes, exact result types, ordered
  operands, typed attributes and admitted leaves; serialization ordinals are
  handles, and verification independently normalizes the live body;
- architecture obligations now use exactly a module-owned
  `ac.arch_obligation` symbol operation referencing a module-owned typed
  expression table, with structural non-content IDs, complete typed sampling,
  `NDEBUG`-independent pre-publication release checks, and proof invalidation;
- the phase-one obligation schema now closes the safety-kind and
  `cpp|gfsim|sva` target enums and defines only `error|fatal`; both fail the
  run/gate, with fatal immediate termination and error-only bounded diagnostic
  continuation after suppressing the guarded mutation/publication;
- four-state values require `(known & z) == 0`, fail unknown applicable
  controls/assertions, reject unsupported operations, and use static SRAM
  live-window `N=1` rather than a depth parameter;
- parameterized RTL emits one readable typed module family or rejects; nominal
  payload examples include their source-owned interface shard and no per-type
  header authority remains; and
- the checklist now treats F0 decisions as closed and lists only explicitly
  deferred later-phase questions.

## Checklist ledger and hard-break ownership

- `checklist-ledger.jsonl` is a reviewable 22-line canonical copy of
  `.omx/ultragoal/checklist-ledger.jsonl`. `PC-F0-001` intentionally remains
  `partial`; its review verdict must change only after reviewer confirmation.
- The hard-break deletion table assigns framework and consumer owners and
  required absence/negative evidence. The consumer ledger owns removal of
  `davo_core.ac`, flat/interface-modules trees, `assembly.py` implementations,
  and other consumer layout artifacts; pyCircuit owns legacy compiler flows,
  package layouts, naming, identity, and preserved topology rejection.
- `docs/gates/logs/` is ignored by default. Both this summary and the ledger,
  plus the corrected historical commands file, must be force-added explicitly
  when the eventual commit is prepared.
