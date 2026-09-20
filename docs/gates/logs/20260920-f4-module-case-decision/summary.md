# F4 finite module case decision evidence

## Outcome

Decision 0276 freezes the executable body and cross-layer carrier for the finite
families accepted by Decision 0275. It refines Decisions 0274 and 0275 while
preserving Decision 0267 structural specialization equality and Decision 0270
source ownership. This is documentation/governance evidence only. Status is
`gap-in-scope`, and family C++/RTL emission remains blocked until the typed
frontend, ACIR/header/link, QueueGraph, and verified PYC family/case carrier plus
the decision's acceptance/negative matrix are implemented.

## Changed contract surfaces

- `docs/rfcs/pyc6-decisions.md`: normative Decision 0276.
- `docs/gates/decision_status_v6.md`: `gap-in-scope` status and implementation
  boundary.
- F4 RFC/checklists: concrete case-region ownership, typed plans, verified PYC
  carrier, and hard-break prerequisites.
- `docs/reference/name-mangling.md`: one family identifier and removal of
  suffix/dictionary/sidecar identity.
- `docs/development/agent-frontend-guide.md`: authoring syntax and contributor
  routing.
- `architect_review.md`: bounded architecture verdict, invariants, risks, and
  deferred boundary.

## Required implementation evidence not yet present

- Python syntax and unparameterized one-empty-case normalization;
- ACIR family/case structure, typed arguments, dependent signatures, and
  case-local ownership verification;
- complete header/import/link case coverage independent of callers;
- typed `ModuleFamilyPlan` and `ModuleCasePlan` records;
- verified PYC family/case carriage before C++ or RTL emission;
- one-identifier C++ explicit case materialization and one RTL
  parameter/generate family;
- acceptance/negative matrix coverage and deterministic repeat emission;
- absence gates for suffixes, dictionaries, per-case symbols/files,
  `specializations.json` sidecars, readers, shims, and dual modes.

## Documentation gates

- `python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit
  examples/pycircuit docs README.md` — passed: `ok: API hygiene check passed`.
- `mkdocs build --strict` — passed; documentation built successfully.
- `python3 flows/tools/check_decision_status.py --status
  docs/gates/decision_status_v6.md --out
  .pycircuit_out/gates/20260920-f4-module-case-decision/decision_status_report.json`
  — expected fail-closed result: unresolved gaps are exactly 0273, 0274, 0275,
  and 0276. The report has no missing decision row, extra row, or invalid
  status.
- `git diff --check` — passed.

The decision-status failure is required evidence that Decision 0276 was not
misrepresented as implemented. It does not indicate a malformed decision
table.
