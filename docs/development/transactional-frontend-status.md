# Transactional frontend replacement scope

Production compile/link uses transactional ACIR v2 with one source-unit route.
The [language reference](../reference/language.md#transactional-frontend-replacement)
owns semantics, the [frontend guide](agent-frontend-guide.md) owns authoring,
and [current capabilities and gaps](known-limitations.md) owns the capability
inventory. The [compiler pipeline](../architecture/compiler-pipeline.md) and
[source-unit workflow](source-unit-workflow.md) describe stage ownership.

## Migration accounting

The replacement reuses source publication, explicit closure, common transaction
analysis, Routed verification, storage lowering and both emitters. The previous
automatic ordinary Rule registration, candidate-state forwarding and writable
Method-parameter behavior are superseded. Old units must be regenerated; new
compilation failures do not fall back to the old frontend.

Focused transactional regressions and examples cover State, Method calls,
parent-owned resources, Queue timing, multilevel references and structured
payloads. The #361 core slice adds static branch output connections, selected
transaction paths and omitted State initialization. The cumulative and four-sample
batch examples preserve their component algorithms; their tests check complete
state/consumption traces, fifth-input blocking and output backpressure. Separate
conditional-Method and four-state reset cases cover path admission and X defaults.
Loops, dynamic instance/resource selection, ordinary local merges and dynamic
unknown control remain gaps. Direct transactional gfsim remains unfinished.
Their independent expectations remain in test-owned models and source
assertions. Example introductions describe their own behavior; they do not
certify the migration of a larger historical design.

| Retained input | Disposition |
| --- | --- |
| Historical examples, source fixtures and independent oracles | Retain until migrated or explicitly retired; presence is not execution evidence. |
| `SourceMemoryBindingsTest.cpp` plan-class cases and entry-wrapper snapshots | Migration inputs; previous NOT RUN records do not establish current results. |
| Old TableQuery write-plan-internals case | Retired implementation-specific coverage, not a passing test. |
| Original `queue_rule_pipeline`, structured examples and full multi-output designs | Preserve original behavioral expectations; scalar/packet teaching examples do not replace their full coverage. |
| Direct transactional gfsim | Remaining implementation work; no alternate production frontend. |

## Validation and evidence

Run the existing API and examples gates described in
[testing and gates](testing-and-gates.md). Each run records the candidate,
command, exit status and selected scope under ignored `docs/gates/logs/<run-id>/`.
Report PASS, FAIL and NOT RUN separately from migrated, retained or retired.
Historical per-PR pass totals and intermediate failures are available in Git
history and the associated candidate evidence; they are not current support
claims. Full gate failures must retain their first failing stage and cannot be
replaced by a focused transactional pass.

The [replacement plan](transactional-frontend-replacement-plan.md) remains the
work sequence and semantic-decision authority. Plans and approvals do not prove
implementation or authorize merge.
