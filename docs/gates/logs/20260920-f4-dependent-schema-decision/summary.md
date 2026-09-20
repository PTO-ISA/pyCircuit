# F4 exact dependent family schema decision evidence

Date: 2026-09-20

## Outcome

Decision 0278 is accepted as the exact dependent-value, logical-type,
source/provenance, and PYC mapping contract refining Decisions 0275-0277. It
freezes arbitrary-precision mathematical arithmetic, representability,
half-open ranges, Queue lanes/rate, projection/layout records, physical and
logical port mappings, shared-ready Queue carriage, implicit clock/reset
origins, complete case signatures, and family/case/import/instance
verification.

The decision status is deliberately `gap-in-scope`. This change supplies
governance only and does not claim that the current family-attribute WIP or
direct-body/dictionary implementation satisfies the contract.

## Documentation updated

- `docs/rfcs/pyc6-decisions.md`
- `docs/gates/decision_status_v6.md`
- `docs/rfcs/ac-cpp-pointer-owned-module-composition.md`
- `docs/rfcs/ac-rule-simqueue-atomic-lowering-checklist.md`
- `docs/rfcs/ac-architecture-rule-rtl-verification-extension-checklist.md`
- `docs/rfcs/architecture-rule-compiler-extension.md`
- `docs/reference/name-mangling.md`
- `docs/development/agent-frontend-guide.md`

No implementation, test, generated artifact, or installed skill is part of
this governance commit.

## Frozen boundaries

- Closed dependent-value and ordered dependent-argument records.
- Exact mathematical arithmetic, width functions, and consumer
  representability.
- Exact logical type-expression union and per-case materialization bounds.
- Typed source owner, interface, and provenance records.
- Complete PYC projection, packed-layout, physical/logical port, control,
  module mapping, and case-signature records.
- Per-lane Queue valid/data plus one shared ready, and explicit clock/reset
  inputs at physical indices 0/1.
- Structural family/case/import/instance verification and deterministic order.
- Atomic removal of legacy dependent literals, strings/postfix paths, inferred
  mapping/layout/control metadata, and partial carriers.

## Validation

- `python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md`
  passed: `ok: API hygiene check passed`.
- `mkdocs build --strict` passed and built the documentation successfully.
- `python3 flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out docs/gates/logs/20260920-f4-dependent-schema-decision/decision_status_report.json --require-concrete-evidence --require-existing-evidence`
  produced a structurally complete report: 278 decisions and 278 rows, with no
  missing/extra decisions, invalid statuses, placeholder/empty evidence, or
  missing evidence paths. It exits nonzero only because Decisions 0273-0278
  remain intentionally `gap-in-scope`; this is the expected fail-closed release
  result for a governance-only change. The bounded result is preserved in
  `decision_status_summary.json`.
- `git diff --check` passed.
