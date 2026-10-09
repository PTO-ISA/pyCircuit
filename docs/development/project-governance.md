# Project governance

pyCircuit is a hardware programming language and compiler with one Python
capture → MLIR analysis/lowering → verified common IR → C++/Verilog flow.
Current support is defined in the [language reference](../reference/language.md).
Frozen approvals under `docs/rfcs/contracts/approvals/` record authority for
semantic interfaces; they do not establish implementation or passing tests.

## Scope and authorization

Follow the user's latest direction and existing authorization. On 2026-10-07
the user ended migration expansion and requested delivery of the retained
supported subset. Uncovered historical examples and API drafts are removed;
removal is not counted as verification. On 2026-10-09 the user reopened example
migration with simple, efficient source authoring and no new APIs. Continue
against the original behavior and independent oracles; repair concrete common-IR
or backend defects without introducing alternate compiler paths. Keep meaningful supported behavior,
rejection, ownership, timing, reset and zero-commit-on-failure tests.

Use descriptive responsibility names instead of migration task or milestone
codes. The current example/API catalog is `examples/catalog.json`. Logs and
historical work/review notes are local ignored artifacts. Runtime tests must
not depend on those notes, their hashes or acceptance counters.

## Ownership and review

The integration owner keeps scope, file ownership, shared registries, build
metadata, candidate identity and final reporting consistent. Preserve others'
work in a shared checkout. Tasks specify inputs, exclusive files, dependencies,
expected behavior, removal scope and the smallest useful checks.

Work directly for small mechanical tasks. Complex semantic changes require a
real architect and an independent decomposition instance before bounded
implementation. Implementation, independent tests and review use separate
instances. Record actual role, model and effort. The requested implementation
and review default is `gpt-6.1-sol`; the architect preset is `gpt-6-astra` at
`xhigh`. A role label does not establish that a review occurred.

Existing approval persists for its scope. A genuinely new semantic interface
needs a concrete proposal and independent review before implementation. Do not
ask for repeated approval of already-authorized reversible work.

## Compiler boundaries

Keep capture thin. MLIR owns type, dependency, effect and hardware inference.
Derive admission from declared types, actual SSA, effects and owners. Example
names, constants, widths or fixture shapes must not become compiler rules.
No compatibility aliases, retired-route fallback, parallel semantic compiler,
backend-only semantic patch or adapter may bypass the verified common IR.

Keep DUT sources focused on hardware algorithm, state and connections. Oracle
models and framework coverage belong to tests and must never compute DUT
results inside product compilation. Consumer designs, schemas and ISA logic
belong in their owning repositories.

## Validation and delivery

Build from this checkout; never copy a compiler, library or generated artifact
from another worktree. Preserve old-state Work, successful whole-system checking
before Xfer, source ownership and output-protection contracts. Both backend
outputs consume the same verified final IR.

Use the existing API/example entrypoints and [test tiers](testing-and-gates.md).
Long reference-model, coverage and mutation matrices belong in nightly and
must not be launched when the user defers them. Record precise commands,
exit status, candidate revision/content binding, failures and skipped checks
in the PR and local `docs/gates/logs/<run-id>/`; CI uploads logs as artifacts.
Historical evidence does not certify the current candidate.
