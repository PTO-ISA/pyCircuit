# pyCircuit 6 Evolution Plan

This plan turns the accepted contracts in
[`pyc6-decisions.md`](rfcs/pyc6-decisions.md) into reviewable work. The V6
language specification is the product contract; the decision corpus records why
that contract exists.

## Ground rules

- Treat `docs/v6_PyCircuit_Specification.md` as the language source of truth.
- Treat `docs/rfcs/pyc6-decisions.md` as the semantic decision source of truth.
- Preserve the CycleAwareSignal model defined by Decision 0148.
- Add or tighten MLIR verification before changing semantics.
- Do not implement semantic fixes in only one backend.
- Build and test from the current worktree.
- Reference affected decision IDs and archive reviewable semantic evidence under
  `docs/gates/logs/<run-id>/`.
- Use `libpyc6_runtime`, `.pyctrace` schema magic `PYC6TRC3`, and
  `run_semantic_regressions_v6.sh` for active products and gates. Preserve only
  historical files under `docs/gates/logs/` as immutable evidence.

## Baseline inherited by V6

The earlier closure evidence remains the verified baseline for Decisions
0001–0147. It covers static-hardware legality, backend equivalence, observation
points, reset and trace semantics, structured interfaces, DFX, incremental
builds, and cosimulation. The active status index is
`docs/gates/decision_status_v6.md`; its older rows retain their original
evidence paths.

V6 adds Decision 0148: the global CycleAwareSignal design is the primary
authoring model. This supersedes Decision 0010 without invalidating the
unrelated earlier closure evidence.

Decision 0150 adds Agentic Circuit as an independent upper-level ACIR and
frontend in this repository. Its synthesizable path converges on verified PYC
IR and the pyCircuit 6 backends; its ACSim/gfsim architecture-simulation path
remains distinct.

## Milestones

### Documentation and governance convergence

**Goal:** Present one current language and one repository authority.

- [x] Make the V6 specification, tutorial, and software architecture the
  product-facing documentation set.
- [x] Supersede Decision 0010 with Decision 0148.
- [x] Remove the duplicate prior-version specification from current navigation.
- [x] Document PTO-ISA/pyCircuit as upstream and LinxISA/pyCircuit as a
  downstream fork.
- [x] Keep automation, issue templates, release metadata, and repository rules
  aligned with the upstream/fork ownership model.

### Public API convergence

**Goal:** Remove versioned implementation vocabulary from the supported API
without removing cycle-aware semantics.

- [x] Make CycleAwareSignal examples and API references consistent across the
  README, tutorial, and reference pages.
- [x] Rename prior-version-labelled modules, comments, diagnostics, and tests where
  the name is not an ABI or serialized compatibility contract.
- [x] Keep `CycleAwareCircuit`, `CycleAwareDomain`, `CycleAwareSignal`,
  `CycleAwareTb`, and `compile_cycle_aware()` supported as defined by the V6
  specification.
- [x] Reject removed compatibility APIs instead of silently accepting them.

### Post-6.0 cycle-aware hardening backlog

Decision 0148's public V6 contract is implemented and verified by the focused
V6 tests plus the examples, simulation, and semantic lanes archived under
`docs/gates/logs/pyc6-unification/`. The items below expand coverage beyond the
6.0 migration baseline and remain follow-up hardening work.

- [ ] Verify cycle provenance across `domain.call()` and `pyc.instance`
  boundaries.
- [x] Verify automatic delay insertion for mixed-cycle arithmetic, comparison,
  mux, vector, and structured signals.
- [x] Verify `domain.signal()` inference for combinational, single-stage, and
  multi-stage assignments.
- [ ] Verify invalid backward-cycle assignments fail with source-located
  diagnostics.
- [x] Compare C++ and Verilator results at TICK-OBS and XFER-OBS.

### Repository and release closure

**Goal:** Make PTO-ISA/pyCircuit the only release and governance authority.

- [x] Require protected review and required checks on the upstream default
  branch.
- [x] Publish releases and packages only from PTO-ISA/pyCircuit.
- [ ] Keep Linx-specific changes mergeable upstream; rebase the LinxISA fork
  from upstream after releases or integration milestones.
- [x] Ensure package metadata, documentation URLs, badges, and source links name
  PTO-ISA/pyCircuit.

### Agentic Circuit consolidation

**Goal:** Make this repository the only development and release authority for
ACIR, Agentic Circuit, and pyCircuit without weakening the CycleAwareSignal or
PYC semantic contracts.

- [x] Import the Agentic Circuit `main` history at
  `756002e2998b11dfe1fed14dc3d63cdad8be694c` with provenance intact.
- [x] Record PTO-ISA's BSD-3-Clause owner direction and the imported main and
  open-PR head objects in `docs/legal/AC-RELICENSE-BSD-3-CLAUSE.md`.
- [x] Migrate the unique work and review disposition from Agentic Circuit PR
  #18 and PR #23 into reviewable pyCircuit changes.
- [x] Integrate the `agentic_circuit` distribution, ACPy/ACIR, ACSim/gfsim,
  and ACIR-to-PYC targets without merging their Python or MLIR namespaces.
- [x] Replace prior-version pyCircuit runtime references with repo-local
  pyCircuit 6 targets and `libpyc6_runtime`.
- [x] Run the ACIR/ACSim verifier and unit lanes, ACIR-to-gfsim execution, and
  ACIR-to-PYC-to-C++/Verilog gates from the consolidated checkout.
- [x] Run the existing pyCircuit 6 API, examples, simulation, and semantic
  closure lanes from the same checkout.
- [x] Attach gate evidence and promote Decision 0150 to
  `implemented-verified` for repository and compiler consolidation.
- [ ] Close Decision 0151 for Agentic Circuit epoch `0.4`: verify the
  one-dimensional zero-initialized single-writer Table through ACPy, Frozen
  ACIR, QueueGraph, and typed gfsim C++, with a stable PYC rejection boundary.
- [x] Close Decision 0152 without changing epoch `0.4`: verify state-driven
  Table updates, match/choose, and committed Queue slots through both typed
  gfsim C++ generators while preserving the same PYC rejection boundary.
- [x] Close Decision 0153 without changing epoch `0.4`: accept a same-Table
  CandidateSet in `table.view(mask)`, retain scalar-index writes, and verify
  atomic uniform masked write/patch through both typed gfsim C++ generators.
- [x] Close Decision 0154 without changing epoch `0.4`: permit multiple Table
  writer endpoints with pairwise-disjoint top-level field sets and merge their
  old-state proposals atomically through both typed gfsim C++ generators.
- [x] Close Decision 0155 without changing epoch `0.4`: lower each authored
  Table match/choose once and reuse its lazy full-Epoch result across all
  direct and native gfsim endpoint consumers.
- [x] Close Decision 0156 without changing epoch `0.4`: add one state-driven
  scalar allocation endpoint whose complete Entry replacement wins over
  same-Entry field writers in direct and native typed gfsim C++.
- [ ] Implement `D-RULE-LOWERING-001`: make `@ac.rule` the simple Python
  scheduling boundary; infer types, effects, checks, handshake, and conflicts
  through staged ACIR passes; preserve incomplete knowledge with typed markers;
  bump the Agentic Circuit contract epoch to `0.5`; and reject every unresolved
  marker before Frozen ACIR topology freeze, hashing, or serialization.
- [ ] After the independent current QEMU/PYC comparison passes, disable the
  old repository's publishing/CI authority and make it
  private with only `zhoubot` as a direct repository collaborator.

## Gate mapping

Use the minimum applicable lanes from
[`testing-and-gates.md`](development/testing-and-gates.md).

| Change | Required evidence |
| --- | --- |
| Documentation or governance | changed-file pre-commit checks, API hygiene, `mkdocs build` |
| Cycle-aware frontend or inference | unit tests, API hygiene, examples, semantic regressions |
| MLIR semantics or legality | examples, normal and nightly simulations, semantic regressions, strict decision status |
| C++ or Verilog behavior | both simulation lanes and backend-equivalence evidence |
| Linx integration | pyCircuit lanes plus the Linx interface and model-comparison gates |
| ACIR or Agentic Circuit integration | ACIR/ACSim verifier and unit lanes, ACIR-to-gfsim, ACIR-to-PYC-to-C++/Verilog, plus pyCircuit examples, simulations, and semantic regressions |

Use one `PYC_GATE_RUN_ID` for related semantic lanes. Record skipped gates and
their risk in the pull request.

## Completion criteria

The pyCircuit 6 transition is complete when:

- current docs contain no prior-version label as the active product language;
- Decision 0148 has focused tests and cross-backend evidence;
- supported examples use the V6 CycleAwareSignal contract;
- repository metadata and release automation point only to PTO-ISA/pyCircuit;
- the LinxISA repository is maintained only as a downstream fork; and
- Decision 0150 has archived AC and PYC closure evidence, and the retired
  Agentic Circuit repository has no publishing or CI authority; and
- all required gates pass from a clean worktree.
