# pyCircuit agent instructions

pyCircuit is a hardware programming language and compiler. Use one Python
capture → MLIR analysis/lowering → verified common IR → C++/Verilog flow.
The active governance is defined below; superseded migration schedules and
milestone checklists are not current delivery policy.

## Read the relevant authority

- [Project governance](docs/development/project-governance.md): scope, ownership and independent review.
- [Repository management](docs/development/repository-management.md): Issue/PR intake, dependencies and delivery.
- [Contributing workflow](docs/development/contributing-workflow.md): affected-layer checklist and implementation workflow.
- [Review and merge](docs/development/review-and-merge.md): findings, feedback and readiness.
- [Testing and gates](docs/development/testing-and-gates.md): exact gate selection and evidence.
- [Language reference](docs/reference/language.md) and [frontend guide](docs/development/agent-frontend-guide.md): current authoring and admission.
- [Source-unit workflow](docs/development/source-unit-workflow.md): compile/link/emit, ownership and stage artifacts.

Read affected specifications and exact semantic approvals under
`docs/rfcs/contracts/approvals/` when changing behavior. A proposal or approval is
not implementation or execution evidence. Preserve frozen technical records;
update active behavior docs when implementation changes.

## Issue and PR delivery

All subsequent pyCircuit issues and PRs, including agent and CLI submissions,
follow the checked-in templates and repository-management flow. Issue titles
use `[Area] summary`. PRs identify related issues/dependencies, affected owners,
actual validation and remaining work. Required main-branch checks are versioned
in `.github/repository-governance.json`; metadata completeness does not replace
semantic checks or independent review.

Continue authorized local work without repeated permission requests. Preserve
other work in shared checkouts and stage only task-owned changes. Verify the
repository, branch, base/head and writable remote before delivery; also verify
`git rev-parse --show-toplevel` when working in a submodule worktree.
Do not post, push, merge or change assignments/projects beyond the user's scope.

Repository-local skills route work without duplicating policy:

- `.codex/skills/pycircuit-project-manager/SKILL.md`: planning and issue/PR delivery.
- `.codex/skills/pycircuit-design-review/SKILL.md`: independent semantic proposal review.
- `.codex/skills/pycircuit-pr-review/SKILL.md`: independent implementation review.

## Hardware and compiler invariants

- Keep capture thin; do not execute design functions. MLIR owns type, dependency,
  effect, ownership and hardware inference using declared types and actual SSA.
- No fixture-specific admission: example names, widths, constants or statement
  recipes must not decide product semantics. Fixed primitive contracts and
  independently derived test values are legitimate.
- No shim or alternate route: repair the owning frontend/IR/codegen implementation.
  Retired CycleAwareSignal/JIT, structural builders, Agentic Circuit/QueueGraph,
  `acc.py`, `acc` and `pycc` are not compatibility fallbacks.
- Work observes committed state; nested transaction candidates follow their
  current specified semantics. Whole-system checks precede Xfer/commit. Preserve
  hold, discard, reset, four-state behavior and zero commit on failure.
- Both targets consume the same verified final artifact. Preserve source-unit
  body/interface authority, explicit link closure and output protection.
- `@system` is a current language feature. Changes affecting it require generated
  C++/Verilog simulation evidence; do not move compiler-owned test scaffolding
  to the author to bypass missing support.
- Keep complete consumer designs, ISA logic, schemas, adapters and comparison
  tooling in their owning repositories. Independent oracles remain test-owned
  and never supply DUT results inside product lowering.

## Verification

Build from this checkout; never copy toolchains, libraries or generated outputs
from another worktree. Runtime-only consumers must not require LLVM; CompilerDev
uses the exact LLVM/MLIR version specified by the active build contract.

Use the narrowest applicable checks, then required candidate acceptance gates.
Keep long oracle, mutation and coverage matrices in existing nightly entrypoints;
respect explicit deferrals. Retain complete required stages through existing
manifests, including API-owned roots. Record commands, exit status, exact content,
evidence paths and NOT RUN cases under ignored `docs/gates/logs/<run-id>/` and in
PR summaries. Tests must not depend on local management logs or review transcripts.
A removed case, fixture name, previous pass or empty run is not current evidence.
