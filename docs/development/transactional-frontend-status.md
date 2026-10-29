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
Module, Rule and Method payload local joins execute through common branch analysis.
Rule/Method early returns normalize to the same structured branch results and
whole-transaction commit. Loops, dynamic instance/resource selection and dynamic unknown
control remain gaps. t-gfsim (transactional-gfsim) supports the bounded Bits State/Queue profile described
in the source-unit workflow; Nominal payload generation remains a gap.
Their independent expectations remain in test-owned models and source
assertions. Example introductions describe their own behavior; they do not
certify the migration of a larger historical design.

| Retained input | Disposition |
| --- | --- |
| Historical examples, source fixtures and independent oracles | Retain until migrated or explicitly retired; presence is not execution evidence. |
| `SourceMemoryBindingsTest.cpp` plan-class cases and entry-wrapper snapshots | Migration inputs; previous NOT RUN records do not establish current results. |
| Old TableQuery write-plan-internals case | Retired implementation-specific coverage, not a passing test. |
| Original `queue_rule_pipeline`, structured examples and full multi-output designs | Preserve original behavioral expectations; scalar/packet teaching examples do not replace their full coverage. |
| t-gfsim (transactional-gfsim) | Linked emission supports closed single-clock Bits State/Queue Systems, Rules and Methods with one worker. Nominal payloads and unknown clock/reset samples remain gaps. |

## Main integration migration accounting

The [main capability migration inventory](main-capability-migration.md) assigns
retained capabilities and their verification owners to the ten follow-up batches.
It records implementation obligations; execution results remain candidate-bound.

Body assertions use ordinary reset masking on every Work evaluation, including
low, held and falling clock epochs. Only guard-known checks receive edge sampling.
The saved-anchor regression preserves an independent low-epoch failure oracle
through physical C++ workers 1/2 and RTL; the transactional failure scenario
checks the same boundary in t-gfsim. Dedicated transactional anchor fixtures do
not replace the retained nominal/report migration oracle.

The local integration of `main@3d37651a` into `issue361@7410b48d` preserves
explicit ordinary Rule registration, autorules, old-Q reads, resource Module
interfaces, payload Method interfaces and old-occupancy Queue admission. It
retains the evolved normalized contracts and Linked continuation; the four
removed importer files and the superseded v2 boundary/rejection fixtures stay
removed. No fallback or new public API is introduced.

| Main input | Current disposition and remaining work |
| --- | --- |
| #363 normalized resource contracts | Reconciled with the later #361 implementation. Duplicate structural/rejection fixtures remain retired; retirement is not PASS. |
| #364/#366 void Modules | Migrated to `CodeGen/transactional-void-modules.test` and four public export cases in `test_void_module_results.py`. Aliases, independent state, discarded child work, explicit registration, all three None-return forms, interface-only compilation and Linked continuation use independent histories. Printer formatting is not an oracle. |
| #364 legacy coverage removed | The added native instance-shape test, duplicate execution combinations and negative/immutability/publication matrices were replaced or removed. Legacy mapping/value-input cases are not verified by the replacements. Pure void exports still have compiler-owned clock/reset ports; the previous no-domain expectation is an implementation gap, not a passing case. |
| #365 aggregate availability and Queue proposals | Common hardware fixes and native tests retained. Current Enum/Struct branch and Queue scenarios execute through transactional analysis, routing and final shared availability verification. Compact selections in the retained legacy transaction lowerer do not establish new-frontend Table support. |
| #365 complete allocation pool | `allocation_repro/{contracts,pool,one,two}.py` and `test_published_allocation_pool_preserves_replay_and_instance_isolation` retain their original workload and oracle. Table state, queries and associated authoring remain pending migration; no reduced pool or raised budget substitutes for them. |
| #367 widths and publication isolation | Common Bits width checks and the scoped fstat test override retained. Resolved transactional operations and physical verification continue to check actual widths. |
| #367 static Method parameters | Five `method_generic` cases now use explicit registration, resource connections and closed Systems. They retain 7/69-bit payloads and X/Z, bind actual receiver widths, check unused Methods and preserve failed outputs. Two old candidate-forwarding writes become two read-only exchanges plus one explicit advance; the old double-write/forwarding oracle is retired, not passed. Root value-port checks become State export or closed Queue-consumption checks; external root Queue support is not claimed. |

The System example build uses the emitted CMake inventory. Declaration-only
units and source-global Rule units need not generate a `.cpp`; the example
helper must not invent one from each Python basename.

The static-parameter supplement covers dependent initialization, direct static
operands, forwarding, default/overridden roots, 80-bit state and parameterized
depth-1/depth-2 Queues. Its independent finite-sample model includes held clocks,
reset and old-occupancy blocking. Provider source/body hiding and source removal
before saved Linked continuation preserve publication independence. These cases
do not migrate the retained Table parameter matrix or resolve external resources.

The full API/example registries retain additional migration failures beyond
Table: legacy automatic registration/value interfaces,
ordinary helpers, Struct defaults and retained anchor/source fixtures. These
remain required work in their existing owners. A smaller passing transactional
selection does not close those gaps. Do not reduce the registries or restore
old semantics to obtain a passing gate.

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
