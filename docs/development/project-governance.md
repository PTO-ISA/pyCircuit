# Project Governance

Status: active for the pyCircuit modernization program as of 2026-09-27.

This document governs planning, delegation, review, and evidence for the
[modernization plan](pycircuit-modernization-plan.md). It activates project
management for the GFSIM-led Pythonic frontend to MLIR to common hardware IR
to one C++ generator and one Verilog backend. It does not approve a product or
interface change, supersede an accepted pyCircuit decision, or authorize an
implementation cutover.

Until an exact replacement contract is approved by the user and recorded in
the decision corpus, all current semantic and interface constraints remain in
force. Governance work and independent investigation may continue while an
interface proposal awaits approval.

## Authority and acceptance

The project manager (PM) is the user's single coordination point. The PM owns
scope, dependencies, file ownership, candidate identity, build health, and
final acceptance. Product semantics remain authoritative in
[`pyc6-decisions.md`](../rfcs/pyc6-decisions.md), and verified gate status
remains authoritative in
[`decision_status_v6.md`](../gates/decision_status_v6.md). This document does
not create a competing decision or status registry.

Work falls into three acceptance classes:

- Read-only investigation, governance work, and reversible maintenance that
  preserves current interfaces may proceed within an approved task.
- Every change to a Python, CLI, IR, cross-module, generated C++, runtime,
  schema, diagnostic, timing, ownership, or error contract requires a precise
  proposal, independent design review, and the user's explicit approval before
  implementation.
- Publishing, destructive actions, and external production changes require
  the authority appropriate to that action.

Governance activation is not interface approval. Donor behavior, including a
GFSIM interface, is not approved merely because it is the modernization design
baseline. Existing decisions that conflict with the target architecture are
superseded only through the C1/C2/C3 process in the modernization plan.

An interface approval packet must define:

- before-and-after examples and exact signatures or schemas;
- types, hardware timing, ownership, error and diagnostic behavior;
- the common IR representation consumed by both C++ and Verilog;
- affected callers and the hard-break deletion set; and
- independent tests, gate commands, and rollback boundary.

Only a proposal independently judged approval-ready is sent to the user.
Material revision invalidates that review and requires another approval.

## Roles and model routing

Roles are logical responsibilities. They need not be persistent agents, but
design and validation, implementation and independent tests, and authorship
and code review must use independent instances.

| Responsibility | Default model | Effort and boundary |
| --- | --- | --- |
| PM | Current main session | Do not switch the user's main model merely for this program. |
| Architecture design | `gpt-6-astra` | `high` or `xhigh`; record a reason when critical semantics warrant `ultra`. |
| Independent design validation and validation strategy | `gpt-6-astra` | Match risk; use an independent instance that does not write the reviewed implementation. |
| Decomposition, code review, and build integration | `gpt-5.6-sol` | `low`, `medium`, or `high` according to risk; a reviewer does not author the reviewed diff. |
| Implementation | `gpt-6-luna` | `low`, `medium`, or `high`; bounded file ownership. |
| Independent tests | `gpt-6-luna` | A different instance from implementation; derive expectations from the approved contract. |
| Cross-layer debugging and performance | `gpt-5.6-sol` | Use `high` for difficult cross-layer failures; begin with a minimal reproducer. |
| Documentation and contract cross-check | `gpt-6-luna` | May report inconsistency but cannot approve semantics. |
| Optional DSH `deepseek-flash` task | Provider-confirmed model | Read-only by default; never substitutes for Astra validation, independent tests, or Sol review. |

Complexity determines the smallest valid organization:

| Class | Typical scope | Minimum organization |
| --- | --- | --- |
| Simple | Repository facts, wording, low-risk mechanical work | PM or one bounded investigator. |
| Local | Repair within one accepted contract | PM decomposition, implementation, independent test, Sol review. |
| Complex | Cross-module, build, or generated-code interface | Sol decomposition/integration, bounded implementation and test lanes, plus Astra when design is involved. |
| Critical semantic | New IR/API, timing, ownership, atomicity, or cross-backend behavior | Precise design, independent Astra review, user approval, implementation and independent tests, Sol review, and architecture conformance. |

If the requested model is unavailable, record the missing responsibility and
adjust the schedule; do not claim that review occurred. Fixed role aliases do
not prove a requested model or effort. Two attempts without new evidence are a
signal to reduce the problem, change the diagnostic method, or add a focused
debugging lane rather than repeat indefinitely.

## Native dispatch bindings

The table above names logical responsibilities, not native `agent_type` presets.
Use `agent_type="default"` with explicit `model` and supported `reasoning_effort`
and bounded context when assigning Luna implementation, test, or documentation
work. For example, a Luna test task uses `model="gpt-6-luna"`; selecting the
fixed `test-engineer` preset does not select Luna.

On the current host, `executor` and `test-engineer` are fixed to
`gpt-5.6-sol` / `medium`; `writer` and `code-reviewer` are fixed to
`gpt-5.6-sol` / `high`; `architect` is `gpt-6-astra` / `xhigh`.
A different intended model/effort uses explicit supported configuration rather
than a conflicting preset. Verify availability before dispatch and record the
actual configuration. The initial governance/bootstrap and baseline lanes used
those fixed Sol presets before this canonical routing was activated; their
work-item records preserve that fact and do not describe them as Luna work.

## Task dispatch and file ownership

The PM freezes the input baseline, dependencies, and writable files before
dispatch. Read the host's live concurrency limit; do not encode it in product
code. Child agents do not recursively assemble teams. Each writable file has
one owner at a time, and shared registries, ODS, pipeline wiring, root CMake,
shared documentation, and snapshot indexes have a single integration owner.

Every task packet records:

```text
Task ID, objective, and non-goals
Checkout, HEAD, dirty overlay, and candidate content binding
Accepted contracts and decision IDs
Dependencies and applicable C1/C2/C3 user approval
Logical role, actual model, and effort
Allowed files, forbidden shared files, and build-output directory
Acceptance criteria, independent oracle, rejection cases, and exact commands
Stop conditions and escalation target
Expected report: result, changed files, logs, exit codes, and residual risk
Shared checkout rule: do not revert others' work and do not recursively delegate
```

Different build directories isolate outputs, not concurrent source rewrites.
Freeze or isolate the source candidate when necessary. Build and validate from
the candidate checkout; never copy a compiler, library, or generated artifact
from another worktree. Use `followup_task` for a completed or idle instance and
`send_message` only to supplement an active task. Independent test and review
may read the same frozen candidate concurrently; a fix creates a new candidate
and invalidates affected evidence.

## Work items, reviews, and evidence

Work items live under [`docs/work-items/`](../work-items/README.md) and move
through:

```text
backlog -> ready -> active -> review -> verified -> done
```

`blocked` and `cancelled` are terminal side states. `ready` requires satisfied
dependencies and an approved contract when one is needed. `verified` requires
fresh evidence from the current candidate. Only the PM marks `done` after
integration and documentation closeout.

Reviews live under [`docs/reviews/`](../reviews/README.md). Record the proposal
or candidate SHA-256, date, logical role, actual model and effort, verdict,
findings, and disposition. A completed agent message or an old passing log is
not current-candidate evidence.

Use the gate tiers and evidence rules in
[`testing-and-gates.md`](testing-and-gates.md). Evidence records the working
directory, candidate identity, toolchain, options, command, exit status,
failures, and bounded log path. Content identifiers bind project evidence only;
they do not become ACIR identity, generated class names, or a public ABI.

## Implementation and hard-break discipline

Prefer deletion, donor reuse, and boundary repair over new wrappers. Do not
spend migration effort restructuring retired files that the approved hard break
will remove. Separate formatting from semantic changes. New handwritten C++,
header, and TableGen files should remain below 600 lines; a justified exception
records its owner and removal condition rather than evading the limit with
include fragments or empty delegates. Split Python by responsibility as well.

The target has one Pythonic capture frontend, MLIR-owned semantic analysis and
lowering, one verified common hardware IR, one GFSIM-led C++ generator, and one
Verilog backend. Python must not become a second compiler, and a backend fix
must not become semantic authority. Semantic changes begin in dialects,
verifiers, or passes and are verified through both backends.

The M5 cutover is one hard-break candidate. After the relevant interfaces are
approved, it removes the old source APIs, lowering and codegen routes, CMake
targets, exports, flags, fallbacks, tests, installed assets, and active docs in
the same change sequence. Historical revisions remain evidence; no old-route
fallback enters the new product route. Consumer CPU, NPU, SoC, payload, trace,
or model-comparison logic stays in the owning consumer repository.

Governance is maintained from observed conflicts, waiting, rework, and missing
evidence. Its success is demonstrated by correctly completing a vertical
slice, not by increasing templates, agents, or approval steps.
