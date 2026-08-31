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
authoring model. This supersedes Decision 0010 without invalidating unrelated
the earlier closure evidence.

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

Use one `PYC_GATE_RUN_ID` for related semantic lanes. Record skipped gates and
their risk in the pull request.

## Completion criteria

The pyCircuit 6 transition is complete when:

- current docs contain no prior-version label as the active product language;
- Decision 0148 has focused tests and cross-backend evidence;
- supported examples use the V6 CycleAwareSignal contract;
- repository metadata and release automation point only to PTO-ISA/pyCircuit;
- the LinxISA repository is maintained only as a downstream fork; and
- all required gates pass from a clean worktree.
