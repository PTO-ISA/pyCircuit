# pyCircuit 6 agent instructions

This repository follows the pyCircuit 6 frontend contract. CycleAwareSignal is
the primary authoring model, and the V6 documents are the current product source
of truth.

## Read first

- `docs/v6_PyCircuit_Specification.md`
- `docs/rfcs/pyc6-decisions.md`
- `docs/pyc6-plan.md`
- `docs/development/contributing-workflow.md`
- `docs/development/testing-and-gates.md`
- `docs/development/review-and-merge.md`

## Codex skills

- Apply `$pyc6` first for hard contracts and evidence expectations.
- Use `$pyc-build-v60` when running builds or gate lanes.
- Consumer-specific compatibility work runs in the owning consumer repository,
  not in this framework tree.

## Task mapping

- Issue fix or feature work: identify affected decision IDs, then map the change
  to the required gates in `docs/development/testing-and-gates.md`.
- Code review: prioritize semantic regressions, missing gate coverage,
  incorrect evidence paths, and documentation drift before style issues.
- PR preparation: include decision IDs, gate commands, evidence paths, doc
  updates, and compatibility or risk notes.
- Documentation updates: keep the V6 specification, contributor docs, README,
  and actual repository workflow aligned.

## Hard rules

- Keep CycleAwareSignal, CycleAwareDomain, and automatic cycle balancing as
  first-class pyCircuit 6 design contracts (Decision 0148).
- Add or tighten MLIR verifiers or passes before changing semantics.
- Do not implement semantic fixes in only one backend. Semantics live in the
  dialect, passes, and verifiers.
- Build and test from the current checkout. Never copy staged toolchains,
  shared libraries, or generated artifacts from another worktree.
- Do not place temporary tests, scripts, examples, or design notes in the repo
  root. Use the existing test, example, documentation, or disposable output
  directories.
- Treat public examples as product surface. New examples must provide
  user-facing design coverage, compile-flow coverage, or semantic evidence.
- Reference affected decision IDs and attach semantic or decision-bearing gate
  evidence under `docs/gates/logs/<run-id>/`.
- Keep the repository hard-break only. Do not restore removed compatibility
  modes or label the current CycleAwareSignal API with a prior product version.
- Keep active runtime, trace, and semantic-gate names on the pyCircuit 6
  contract: `libpyc6_runtime`, `PYC6TRC3`, and
  `run_semantic_regressions_v6.sh`.
- Keep complete CPU/NPU/SoC/board designs, consumer testbenches, ISA decoders,
  model-comparison scripts, and consumer-specific runtime adapters out of this
  repository (Decision 0158).
- Do not add AI co-author lines to commits or pull request text.

## Repository authority

- `PTO-ISA/pyCircuit` is the upstream source of truth and release authority.
- `LinxISA/pyCircuit` is a downstream framework-compatibility fork, not the
  owner of Linx design or integration sources.
- Product decisions and reusable framework fixes land upstream. Consumer
  compatibility gates run from the consumer checkout against a pinned
  revision.
- See `docs/development/repository-management.md` for branch, release, and fork
  synchronization policy.

## When to stop and ask

- The requested change conflicts with an accepted pyc6 decision.
- The work would change documented semantics without a clear decision update.
- Unrelated user changes overlap the same files and the merge strategy is
  ambiguous.
- Required credentials or external tooling block required validation or
  publishing.

## Working expectations

- Start with the smallest reproducer and narrowest gate lane that proves the
  change; widen only as required by risk.
- Keep generated logs bounded and archive only reviewable evidence.
- Update behavior documentation in the same change as the behavior.
- Report non-critical local validation gaps explicitly instead of hiding them.
