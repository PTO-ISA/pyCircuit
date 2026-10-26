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

## Pythonic authoring contract

The user requests Astra (`gpt-6-astra`) for delegated pyCircuit source authoring
and authoring-contract review. Keep compiler-only implementation routing separate.

For source authoring, first read [.codex/skills/pycircuit-authoring/SKILL.md](.codex/skills/pycircuit-authoring/SKILL.md).
Write function calls, ordinary local computations and sequential Rule assignments.
Directly nested Rules register automatically once per Module instance. Their
names denote results at Module wiring sites; do not call them explicitly.
Source-global Rule templates receive explicit arguments. Callable component
operations use Methods, independently of their parameter count.
Module variables own persistent state; ACIR supplies storage and its controls.
Ordinary example/benchmark frontend sources must not expose clk/reset, register
q/d interfaces, DFF/DFFE bindings or proposal/commit plumbing. Original RTL names
and physical ports belong at the compiler export or test adapter boundary.
Single values use scalar arguments/results; do not wrap one ac.u8/Bits/bool in a
one-field Struct or singleton tuple. Structs group multiple related fields.
Dedicated primitive/nominal-type conformance tests retain their explicit test
purpose and must not be copied as ordinary design style.

## Module output authoring

Choose module interfaces by how their fields are connected. When several fields
normally travel together to one downstream module, prefer a named Struct as one
output and one typed input on the consumer. Pass that structured value directly;
avoid flattening it into parallel ports and rebuilding it at the next boundary.
Adding or removing a field should normally update the shared Struct definition
and affected field users, not the inter-module argument/result lists.

Use `-> tuple[T0, T1, ...]`, ordered returns and direct unpacking for independent
interfaces or outputs routed separately. Each tuple element may itself be a
Struct. A real structured connection is encouraged; do not impose one wrapper
Struct/NamedTuple or mapping over unrelated output interfaces. Preserve instance
identity, timing and existing oracles.
Follow the [frontend authoring contract](docs/development/agent-frontend-guide.md#module-hierarchy-and-independent-outputs)
and [multi-output example](examples/multi_outputs/README.md). This requirement
supersedes blanket advice to either bundle all outputs or split every field.

## Rule authoring requirements

Agents must follow the [Rule authoring contract](docs/development/agent-frontend-guide.md#rule-authoring-contract).
Keep each Rule simple and cohesive: one function or one set of effects that must
commit together. Put independent functions in separate Rules; preserve required
atomicity when splitting. A Rule may be defined inside its Module and has no
persistent local state. Use typed arguments and the same multi-output calling
style as Modules: ordered tuple results with direct unpacking for independent
interfaces, and named Structs for fields of one shared interface.

Leave resource admission and proposal/commit guards to compiler analysis and
Rule lowering. Do not require authors to repeat full/empty/ready or output
acceptance checks already specified by the called interface. Keep actual
algorithmic predicates explicit. The compiler generates and propagates ordinary
output availability; do not add a payload validity field, replacement zero or
holding register solely to represent a Rule that did not commit. Protocol fields
such as a queue's raw valid signal remain part of that interface. The standalone
Signal resource design is cancelled: use typed Module/Rule inputs and results
with compiler-owned availability, and state/Queue for cross-cycle storage.
Do not introduce Signal declaration/set/get APIs or restore that task. Never infer protocol semantics from signal
names, delete necessary checks before equivalent lowering exists, or claim
unsupported Rule syntax works. Record such gaps under issue #319; the authoring
requirement does not establish implementation support.

Cross-Module Method calls target an existing Module instance. MLIR owns the
additional ports and hierarchy wiring needed for arguments, results and atomic
control; authors must not hand-build forwarding/handshake wrappers. Preserve the
callee's state identity and include its proposals in the caller's transaction.
Use the supported static instance and callable forms documented in the frontend
guide; retain independent compile/link and generated System coverage.

## Module state declaration checks

Treat module-scope persistent variables as compiler-generated DFF/DFFE storage
children, using the same `ac.instance` and typed ports as other child modules.
Require coverage of declared Bits, Enum, Struct and arithmetic data types with
defined storage representations; do not restrict this principle to `u1`–`u63`.
Preserve logical types, ownership, initial/reset values and update conditions.
Ordinary expression locals and child-result bindings remain wires, not storage.
Follow the [required state-declaration checks](docs/development/agent-frontend-guide.md#required-state-declaration-checks).
Report unsupported type/path combinations as implementation gaps; a handbook
requirement is not evidence that those combinations already compile and execute.

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
