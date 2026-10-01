# pyCircuit 6 agent instructions

This repository follows the pyCircuit 6 frontend contract. CycleAwareSignal is
the primary authoring model, and the V6 documents are the current product source
of truth.

## Read first

- `docs/development/agent-frontend-guide.md`
- `docs/reference/language.md`
- `docs/rfcs/pyc6-decisions.md`
- `docs/pyc6-plan.md`
- `docs/development/contributing-workflow.md`
- `docs/development/testing-and-gates.md`
- `docs/development/review-and-merge.md`

## Codex skills

- Apply `$pyc6` first for hard contracts and evidence expectations.
- Use `$pyc-build-v60` when running builds or gate lanes.
- Consumer-specific compatibility and design work runs in the owning consumer
  repository, never in this framework tree (Decisions 0158 and 0235).

## Modernization project management

- Follow `docs/development/project-governance.md` and
  `docs/development/pycircuit-modernization-plan.md` for modernization work.
- Use the repo-local `$pycircuit-project-manager` skill for planning,
  dispatch, integration, and acceptance, and `$pycircuit-design-review` for an
  independent approval-readiness review.
- Governance activation does not approve an interface change or supersede a
  decision. Every Python, CLI, IR, cross-module, generated C++, runtime,
  schema, diagnostic, timing, ownership, or error-contract change requires the
  user's precise approval before implementation.
- Keep design and validation, implementation and independent tests, and author
  and reviewer as independent instances. Preserve current semantic constraints
  until an approved contract explicitly cuts them over.

## Hardware design and task boundaries

- pyCircuit is a hardware design language. Designs, modules, register references,
  stateless rules and test systems are the product concepts. A selected test
  fixture or an unfinished implementation slice is not the language definition.
- **NO HARDCODE:** do not use an example's name, width, initial value, increment,
  mask, node count, statement layout or number of ports to decide product
  semantics or admission. Derive behavior from declared types, actual SSA,
  effects, register ownership and approved operation semantics. Hardware source
  constants, independently derived oracle values and fixed primitive contracts
  are legitimate; scenario recognition as a compiler rule is not.
- **NO SHIM:** no compatibility aliases, old-route fallback, parallel semantic
  compiler, backend-only semantic patch, or adapter that bypasses common IR
  inference/verification. Rename or replace the owning implementation and its
  callers together. A temporary workaround needs removal before acceptance;
  calling it internal does not exempt it.
- For a complex change, a real `architect` agent establishes the design and an
  independent decomposition agent splits it into small tasks before execution.
  Each task fixes its inputs, exclusive files, dependencies, expected hardware
  behavior, minimal gates and removal scope. Executors implement those tasks;
  they do not redesign the framework while chasing a failing test.
- PM integrates shared registries and CMake. Implementation, independent tests
  and review use separate instances. Default implementation and code-review
  model is `gpt-6.1-sol`; record the actual role/model/effort. Use the real
  architect preset for architecture, not another role described as architect.
- Remove tests that freeze incidental recipes or duplicate another gate.
  Preserve meaningful ownership, type/range, old-Q, Xfer/hold/discard/reset,
  zero-commit-on-failure, source-unit and output-protection oracles. Never weaken
  hardware semantics to make an example pass, or claim broad migration from
  one or two small examples.

## Task mapping

- Complex circuit authoring: choose the frontend and decomposition pattern in
  `docs/development/agent-frontend-guide.md` before writing implementation.
- Issue fix or feature work: identify affected decision IDs, then map the change
  to the required gates in `docs/development/testing-and-gates.md`.
- Code review: prioritize semantic regressions, missing gate coverage,
  incorrect evidence paths, and documentation drift before style issues.
- PR preparation: include decision IDs, gate commands, evidence paths, doc
  updates, and compatibility or risk notes.
- Documentation updates: keep the V6 specification, contributor docs, README,
  and actual repository workflow aligned.

## Hard rules

- Author product behavior through the supported Python frontends. Handwritten
  PYC or ACIR is valid as focused compiler test input, not as the implementation
  of a user-facing circuit.
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
- Structured Agentic Circuit output is a source-linked AC package. Every
  executable H1/H2/H3 Python source must be compiled by its own CMake custom
  command and direct `acc.py -c <source>.py -o <source>.ac` invocation before backend
  codegen; the root is compiled separately from composition source. A
  whole-core compile followed by either AC or C++ splitting is forbidden.
- Preserve the AC unit boundary through C++: one generated source group per Python source
  `.ac`, plus core/interface glue, compiled independently and linked by parallel
  CMake/Ninja. Gate the AC tree, definition-to-file map, instance links, C++
  tree, build graph, and executable DUT together.
- Keep the active runtime and semantic-gate names on the pyCircuit 6 contract:
  `libpyc6_runtime` and `run_semantic_regressions_v6.sh`. Serialized trace
  formats are tooling artifacts, not public model or runtime ABIs.
- Keep complete CPU/NPU/SoC/board designs, consumer testbenches, ISA decoders,
  model-comparison scripts, consumer payload/trace schemas, and
  consumer-specific runtime adapters out of this repository (Decisions 0158
  and 0235). Framework semantics remain design-neutral.
- Do not add AI co-author lines to commits or pull request text.

## Repository authority

- `PTO-ISA/pyCircuit` is the upstream source of truth and release authority.
- Product decisions and reusable framework fixes land upstream. Consumer
  compatibility gates run from the consumer checkout against a pinned
  revision. Product design, source comparison, and reference-model validation
  stay in the owning consumer checkout.
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
