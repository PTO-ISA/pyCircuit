# Review And Merge

This page defines the review standard for merge-ready pyCircuit pull requests.
It complements the active CI workflows; it does not replace reviewer judgment.

## A merge-ready PR must answer

1. What changed?
2. Which decisions or contracts are affected?
3. Which gates ran?
4. Where is the evidence?
5. What docs changed?
6. What risk or compatibility impact remains?

Use the pull request template to answer these directly in the PR body.

## Review priorities

Reviewers should prioritize:

- semantic regressions or undocumented semantic changes
- missing MLIR-side enforcement for behavioral changes
- missing or incorrect gate coverage
- stale or missing evidence paths
- contributor documentation drift
- hard-break violations such as compatibility shims or competing timing models

Style and cleanup are secondary to correctness, legality, and evidence.

## Current blockers

The following are merge blockers when relevant to the change:

- failing `G0: Python Checks` or `G0: Agentic Python Checks`
- missing focused native, MLIR, runtime, or backend evidence for the changed
  contract
- missing decision IDs for semantic or decision-bearing changes
- missing evidence under `docs/gates/logs/<run-id>/` for semantic or flow
  changes
- missing documentation updates when behavior or workflow changed
- unresolved reviewer concerns about semantics, legality, or reproducibility

For docs-only or template-only changes, the expected blocker set is narrower:
the docs must build, references must resolve, and the workflow text must match
the actual repo commands and paths.

Release-class toolchain, AC G0/G1/G2, cross-backend, nightly, and packaging
lanes are not required PR checks. They run as one release closure and block all
package publication rather than every review iteration.

## When to update decision status or gate docs

Update decision-facing documentation when the PR changes:

- the implementation status of a decision
- the evidence path used to justify implemented status
- the required gate lane for validating a contract
- contributor guidance about how semantic closure is demonstrated

Do not rename historical files under `docs/gates/logs/`. The active decision
corpus and status index are `docs/rfcs/pyc6-decisions.md` and
`docs/gates/decision_status_v6.md`; older artifact names remain evidence only.

## Merge hygiene

- Keep PRs scoped to one change family when possible.
- Separate governance or documentation cleanup from semantic/compiler changes
  unless the docs are part of the same fix.
- Do not merge a semantic change that relies on reviewer guesswork to locate the
  evidence.
- If a gate is intentionally skipped, record the reason and the risk in the PR.
