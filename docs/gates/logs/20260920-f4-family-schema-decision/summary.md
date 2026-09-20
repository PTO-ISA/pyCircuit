# F4 exact finite-family schema decision evidence

Date: 2026-09-20

## Outcome

Decision 0277 is accepted as the exact implementation contract refining
Decisions 0267, 0270, and 0274-0276. It freezes the Python AST, static
configuration and dependent-type rules, typed ACIR AttrDefs, family/case
operation structure, case-local ownership, canonical PYC carrier, and the
one-stage hard break.

The decision status is deliberately `gap-in-scope`. This change supplies
governance only and does not claim that the current direct-body/dictionary
implementation satisfies the new contract.

## Documentation updated

- `docs/rfcs/pyc6-decisions.md`
- `docs/gates/decision_status_v6.md`
- `docs/rfcs/ac-cpp-pointer-owned-module-composition.md`
- `docs/rfcs/ac-rule-simqueue-atomic-lowering-checklist.md`
- `docs/rfcs/ac-architecture-rule-rtl-verification-extension-checklist.md`
- `docs/rfcs/architecture-rule-compiler-extension.md`
- `docs/reference/name-mangling.md`
- `docs/development/agent-frontend-guide.md`

The installed `pyc6` skill is outside this repository and was intentionally not
edited. No repo-backed copy exists in this checkout.

## Frozen boundaries

- Exact tuple-literal Python signatures and child-call syntax.
- Closed static literal/config and dependent expression grammars.
- Dedicated typed ACIR family attributes; no dictionary/JSON/string authority.
- Container-only `ac.module` and ordered non-symbol `ac.module.case` regions.
- Complete typed imports/instances and direct case returns.
- Full case-local ownership/proof keys.
- Typed `pyc.module` family/case carrier with explicit logical-to-physical maps.
- Atomic deletion of direct-body, dictionary, suffixed-symbol, legacy JIT,
  sidecar, and string-parameter paths.

## Validation

- `python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md`
  passed: `ok: API hygiene check passed`.
- `mkdocs build --strict` passed and built the documentation successfully.
- `python3 flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out docs/gates/logs/20260920-f4-family-schema-decision/decision_status_report.json --require-concrete-evidence --require-existing-evidence`
  produced a structurally complete report: 277 decisions and 277 rows, with no
  missing/extra decisions, invalid statuses, placeholder/empty evidence, or
  missing evidence paths. It exits nonzero only because Decisions 0273-0277
  remain intentionally `gap-in-scope`; this is the expected fail-closed release
  result for a governance-only change. The bounded result is preserved in
  `decision_status_summary.json`.
- `git diff --check` passed.
