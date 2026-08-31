# Contributing Workflow

pyCircuit is a hardware design and compile-flow repository. Changes are expected
to preserve semantic contracts, keep gate evidence current, and stay aligned
with the current pyCircuit 6 CycleAwareSignal authoring contract.

## Core principles

- Read the decision corpus before changing semantics:
  - `docs/rfcs/pyc6-decisions.md`
- Follow gate-first development. If semantics change, add or tighten the MLIR
  verifier or pass path before relying on backend behavior.
- Build and validate from the current worktree. Never copy toolchains or shared
  libraries from another checkout.
- Keep the repo hard-break only. Do not add compatibility shims for removed
  APIs or reclassify CycleAwareSignal as a legacy surface.

## Standard development loop

1. Identify the user-visible contract or decision IDs touched by the change.
2. Localize the change to the smallest affected subsystem.
3. Update documentation if behavior, workflow, or contributor expectations
   change.
4. Run the minimum gate set required by the change class.
5. Archive evidence under `docs/gates/logs/<run-id>/` when the change affects
   semantics, flow behavior, or merge-significant examples.
6. Summarize the change, gates, evidence, and residual risk in the PR.

## Blocking vs non-blocking problems

Stop and ask for direction when:

- the requested change conflicts with the decision corpus
- the work requires changing semantics without a clear decision update path
- unrelated local edits overlap the same files and the correct merge strategy is
  unclear
- required credentials or external infrastructure are missing

Proceed and document clearly when the problem is non-blocking, such as a missing
optional gate in the local environment or a known unrelated CI failure.

## Documentation expectations

Update docs in the same change when you alter:

- contributor workflow
- gate expectations
- user-facing CLI or compile-flow behavior
- example structure or supported testbench behavior

Documentation belongs under `docs/`. Do not create standalone Markdown files in
the repo root for design notes or temporary proposals.

## Tests, examples, and temporary artifacts

- Tests validate correctness and regressions.
- Examples demonstrate supported usage and product-facing flows.
- Docs explain behavior and workflow.

Do not add temporary scripts like `test_quick.py`, scratch examples, or one-off
Markdown notes. Keep experimental artifacts outside the repo or under disposable
output directories such as `.pycircuit_out/`.

## Commit preparation

Before opening a PR:

- review the final diff for unrelated churn
- ensure the required gate set ran
- collect evidence paths
- note any compatibility or rollout impact
- use a focused commit message, preferably `type(scope): description`
