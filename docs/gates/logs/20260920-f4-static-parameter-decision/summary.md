# F4 static parameter family decision evidence

## Outcome

Decision 0275 freezes the architect-approved static parameter family schema and
refines Decisions 0270 and 0274 while preserving Decision 0267 structural
specialization equality. This is documentation/governance evidence only.
Implementation status is `gap-in-scope`, and C++/RTL family emission remains
blocked until the typed frontend, ACIR/link, and QueueGraph records plus the
decision's acceptance/negative matrix are implemented.

## Changed contract surfaces

- `docs/rfcs/pyc6-decisions.md`: normative Decision 0275.
- `docs/gates/decision_status_v6.md`: `gap-in-scope` status and implementation
  boundary.
- F4 RFC/checklists: contributor routing and fail-closed emission prerequisite.
- `docs/reference/name-mangling.md`: readable family target and current gap.
- `docs/development/agent-frontend-guide.md`: author/agent routing.
- `architect_review.md`: bounded architecture verdict and deferred boundary.

## Required implementation evidence not yet present

- frontend and ACIR positive/negative schema verification;
- package-link ownership and schema matching;
- typed QueueGraph family plan records;
- one readable C++ template family and one RTL parameter/generate family;
- exact finite-case coverage and deterministic emission;
- per-case narrow/wide Queue storage selection;
- negative scans for suffix/string identity, dictionary declarations,
  caller-observed inference, per-case symbols/files, and dual modes.

## Documentation gates

- `python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit
  examples/pycircuit docs README.md` — passed: `ok: API hygiene check passed`.
- `mkdocs build --strict` — passed; documentation built successfully.
- `python3 flows/tools/check_decision_status.py --status
  docs/gates/decision_status_v6.md --out
  .pycircuit_out/gates/20260920-f4-static-parameter-decision/decision_status_report.json`
  — expected fail-closed result: unresolved gaps are exactly 0273, 0274, and
  0275. The report has no missing decision row, extra row, or invalid status.
- `git diff --check` — passed.

The decision-status failure is required evidence that Decision 0275 was not
misrepresented as implemented. It does not indicate a malformed decision table.
