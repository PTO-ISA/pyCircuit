# Main capability migration inventory

This is a source inventory for the ten bounded #361 replacement batches, based
on `main@3d37651a` and integration base `e22eca28`. It is not execution evidence
or a claim of support. All checks listed here are **NOT RUN for this inventory**.
The current implementation scope remains in [known limitations](known-limitations.md).
The [replacement plan](transactional-frontend-replacement-plan.md) and issue #361
own semantics: committed old-Q reads, explicit ordinary Rule registration,
old-occupancy Queue admission, resource Module interfaces and by-value Method
parameters. No batch restores candidate forwarding or writable parameters.
No t-gfsim capability expansion is assigned by this inventory.

The inventory includes `tests/compiler/IR/CMakeLists.txt`, the lit Source/IR/CodeGen
registries, `tools/run_api_tests.sh` (including its nightly pytest list),
`examples/catalog.json` including `api_coverage`, and `examples/CMakeLists.txt`.
Retained but unregistered fixtures are migration inputs, not gate coverage.
Paths below are actual owners or existing entrypoints; they do not imply that
those entrypoints currently pass through the replacement.

| Batch | Compatible capability and completion boundary | Implementation owners | Existing verification owners |
| --- | --- | --- | --- |
| 1: native anchors/interfaces/tests | Reconcile native source/interface fixtures with explicit registration and resource interfaces; preserve independent compile, explicit link closure, header/body authority, saved Linked/Routed/Final continuation, output protection, void Modules, independent outputs and unique instance ownership. Replace obsolete plan/IR assertions with observable contracts; retain missing capabilities explicitly. | `compiler/lib/Compiler/{SourceUnit,SourceLink,AnchorArtifact,SourceEntryBoundary}.cpp`, `compiler/lib/Dialect/ACIR/{SourceUnitValidation,FinalSourceUnitValidation,TransactionalRouting}.cpp`, `python/pycircuit/{_source_unit_files,_anchor_files}.py` | `tests/compiler/IR/{SourceInterfacePass,SourceUnitHeader,SourceUnitReview,SourceBodySnapshots,SourceLinkAdmission,RoutedAnchor,AnchorArtifact,FinalSourceUnits}Test.cpp`; `tests/system/test_{source_unit_pair_verification,saved_anchor,saved_anchor_variants,void_module_results}.py` |
| 2: Integer/static parameters | Preserve exact Integer arithmetic separately from bounded hardware integers; root required/default/keyword/override bindings, dependent defaults and widths/depth/initializers, forwarded imported parameters, distinct specializations and static Method parameters bound to the actual receiver, including unused Methods. Derive changed Method histories from old-Q. | `compiler/lib/Compiler/{PythonImportContext,SourceHeaderRegistry,SourceLink,NumericLowering}.cpp`; `compiler/lib/Dialect/ACIR/{ACIRStaticEvaluation,ACIRStaticContracts,SourceStaticDomains,SourceStaticValueContracts}.cpp` | `Source/{source-exact-integers,dependent-width-binding,static-divrem,source-historical-scalar-parameterized}.test`; `tests/compiler/IR/{SourceStaticEvaluation,SourceStaticMatching,ExactIntegerLowering}Test.cpp`; `tests/system/test_{scalar_generic_modules,stateful_static_parameters}.py` |
| 3: helpers/expressions | Acyclic ordinary pure helpers with arguments/results, imported helper authority, local computations, arithmetic and Boolean expressions, fixed signedness/width rules, shifts/slices/concatenation, count/priority operations, predicates, selects/case and four-state propagation. Reuse existing value semantics rather than creating a second evaluator. Preserve assertion/observation effects in their owning callable contexts. | `compiler/lib/Compiler/{PythonImportExpressions.inc,PythonImportStatements.inc,PythonImportCaseSyntax.cpp,NumericLowering.cpp,FixedUnsignedDivRemLowering.cpp}`; `compiler/lib/Dialect/ACIR/{SourceHelperAnalysis,SourceValueSemantics,ACIRBitsOps,ACIRMathOps}.cpp` | `Source/{pure-scalar-helpers,bits-*,fixed-*,case-statements,source-exact-select*,source-exact-shift*,source-local-bindings*,source-checks,source-observation-*}.test`; `tests/compiler/IR/{SourceHelperAuthority,SourceValueSemantics,SourceMathContracts,HardwareSourceChecksExecution}Test.cpp` |
| 4: Struct defaults/fields | Nominal Enum/Struct identity, nested constructors, defaults/keywords, imported interface-only construction, field projection and immutable reconstruction for local/State field updates; preserve leaf types, X/Z, initialization/reset and atomic whole-Rule behavior. Keep independent tuple interfaces separate from shared Struct connections. | `compiler/lib/Compiler/{PythonImportRecords.cpp,PythonImportEnumSyntax.cpp,PythonImportExpressions.inc,PythonImportStatements.inc}`; `compiler/lib/Dialect/ACIR/{SourceStaticRecordConstructors,ACIRPacketTypes,ACIREnumOps}.cpp`; `compiler/lib/Transforms/SimplifyRecordWires.cpp` | `Source/{defaults-composition,enum-source,record-projection-simplification,callable-tuple-outputs}.test`; `CodeGen/{transactional-structured-packet,nested-struct-*,struct-wide-*}.test`; `tests/system/test_{source_unit_packet,enum_source_units,boolean_record_fields}.py` |
| 5: loops/static instances | Bounded elaborated loops and callable loops with local SSA/early exits as admitted by retained contracts; static repeated child allocation, instance identity and parameter forwarding; preserve atomic effects across a loop. Static instance choice is not permission for dynamic resource selection. | `compiler/lib/Compiler/{PythonImportStatements.inc,SourceLink.cpp}`; `compiler/lib/Dialect/ACIR/{TransactionalAnalysis,TransactionalResourceConstraints}.cpp`; common control normalization | `Source/bounded-for.test`; `tests/compiler/IR/SourceControlFlowTest.cpp`; `tests/system/test_{rule_scf_control,module_statement_parity}.py`; retained `tests/fixtures/historical_examples/{module_loop,loop_control_pipeline}`; original `examples/queue_rule_pipeline` |
| 6: Table storage | Fixed Table types/layouts, initialization/reset, old-Q indexed reads, whole-element and nested-field writes, copies and aggregate transport. Preserve old state when blocked/discarded; prove update ownership and do not inherit legacy candidate forwarding. Dynamic-address multiwriter proof limitations remain explicit. | `compiler/lib/Dialect/ACIR/{ACIRCollectionOps,TransactionalAnalysis,TransactionalResourceConstraints}.cpp`; importer expression/statement owners; `compiler/lib/Transforms/TransactionSelectedWrite*`; existing storage refinement and emitters | `IR/{collection-constructs,collection-struct,table-region-type-identity}.mlir`; `CodeGen/{collection-constructs-*,collection-struct-cpp,aggregate-table-field-transport}.test`; `Source/table-methods.test`; `tests/system/fixtures/source_system/hierarchical_table_{provider,bench}.py`; `tests/unit/test_table_rule_bench.py` |
| 7: Table queries | Preserve query/reduction/map families, callback binding and pure captures, masks, stable selection/ties, empty-result validity, exact key widths, four-state data and selected-value transport. Cover the retained query families rather than only the pool's argmin. Query evaluation observes old-Q; effects belong to the enclosing transaction. | `compiler/lib/Compiler/{TableQueryLowering,TableQueryStages}.cpp`; `compiler/lib/Dialect/ACIR/ACIRCollectionOps.cpp`; helper and collection type owners | `tests/compiler/IR/{TableQuerySource,TableDependencySummary}Test.cpp`; `Source/table-methods.test` and `Source/Inputs/table-queries/{table_query_cases,table_query_admission}.py`; retained `historical_examples/{array_scans,array_reductions,array_combinators}` |
| 8: full AllocationPool | Preserve complete allocate/duplicate/refusal, lookup/reachability/stale, release/reuse/exhaustion, response routing, replay and two-instance isolation workload. Rewrite the original source with explicit Rule registration and current Queue resources/admission. Do not replace it with a smaller pool or raise validation budgets. | `tests/system/fixtures/source_system/allocation_repro/{contracts,pool,one,two}.py` plus the owning Table/query/transaction fixes | `tests/system/test_rule_queue_external_pop.py::test_published_allocation_pool_preserves_replay_and_instance_isolation`; its existing independent history and workload remain the acceptance owner |
| 9: Memory | Preserve existing primitive/runtime/storage foundations, synchronous and asynchronous reads, initialization/reset, read latency, read-during-write policy, banking/arbitration and atomic failure/discard behavior. Migrate source Methods and interface-only providers after resolving only genuinely unspecified transactional timing. | `compiler/lib/Dialect/ACIR/ACIRCollectionOps.cpp`; common transaction memory analysis/proofs; `compiler/lib/Transforms/TransactionMemory*`; existing storage refinement, runtime and C++/RTL emitters | `Source/pythonic-memory.test`; `CodeGen/{collection-memory-*,sync-memory-*}.test`; `IR/{collection-memory,sync-memory,async-memory-feedback}.mlir`; retained `SourceMemoryBindingsTest.cpp`; `tests/system/test_memory_rule_calls.py`; `examples/{memory_banks,dma}` |
| 10: complete examples/publication/install/primitives | Close every catalog/API-coverage row and historical fixture disposition; restore complete algorithms and independent histories. Verify public compile/link/emit/run, emitted inventories/provenance, relocation, atomic publication/recovery, wheel/Runtime-only/CompilerDev installation and primitive conformance. Remove superseded production callers after their replacements work. | `examples/catalog.json`, `examples/CMakeLists.txt`, `cmake/PycircuitExamples.cmake`, `cmake/verify_example.py`; `python/pycircuit/{_publication,_emit,_run,packaged_toolchain}.py`; packaging/runtime/primitive owners | `tools/run_{api_tests,examples}.sh --tier gate` and retained nightly; `tests/system/test_{public_emit,runtime_install,wheel_install,relocated_compiler,publication_process_recovery,physical_rtl_export,source_system_execution}.py`; `tests/integration/sdk-install`; existing primitive CTest/lit owners |

Source/lit paths abbreviated as `Source/`, `IR/`, `CodeGen/` are under
`tests/compiler/lit/`. Brace/wildcard notation identifies existing families,
not a shell command or a promise to add every combination as a new test.

Dependencies are capability dependencies, not a mandate to serialize unrelated
implementation: 1 establishes native/interface confidence; 2 and 3 support 4/5;
6 needs relevant types, expressions and common update analysis; 7 builds on 6
and helper purity; 8 requires 4, 6 and 7 plus existing Queue/Method behavior.
9 reuses the earlier interface/common-analysis foundations and can proceed
independently of 8 once its timing decisions are resolved. Batch 10 closes the
integrated candidate; each earlier batch still migrates its affected examples,
tests and docs instead of deferring all cleanup to 10.

The catalog's complete-design obligation includes `rob`, `issue_queue_2picker`,
`table_rule`, `fifo_loopback`, `queue_rule_pipeline`, `struct_transform`,
`enum_payload_pipeline`, `digital_filter`, `digital_clock`, `dodgeball_game`,
`bf16_fmac`, `calculator`, `dma` and `memory_banks`, alongside all already
transactional examples and sibling/reference cases. The API-coverage obligation
also includes the named memory roots, reorder/route-merge pipelines, typed Integer
operations, reset/invalidate/trace/XZ cases, credit/expect pipelines, circular ROB,
oldest-ready ISQ, dependency/schedule variants, scalar-parameterized types,
slot mailbox/issue and the complete routed dependency topology. Their existing
`root_scope`/`system_scope` limits in `examples/catalog.json` must remain visible;
a renamed fixture or reduced teaching example does not close an original root.

No compatible capability is deliberately left outside the ten batches. The
following cross-cutting obligations need explicit tracking across batches:

- Arithmetic storage representations beyond Bits/Enum/Struct, including retained
  BF16 behavior, belong to 2/3/4 and their complete designs in 10. Existing primitive
  execution is not evidence of transactional module-state admission.
- Whole-system failure, zero commit, log publication, reset, four-state payloads,
  consumption counts and output availability accompany every affected batch.
  Existing generated post-failure observation and Queue-reset gaps remain gaps.
- `ac.report` is a retained batch-10 source/runtime obligation: the current
  importer handles logs but does not admit report. Preserve the legacy report
  oracle in `tests/system/test_saved_anchor.py` and relevant
  `test_source_system_execution.py` cases; a new log-only anchor scenario does
  not replace it.
- Source maps, identifiers, Unicode names, incremental dependency publication,
  explicit source-unit closure and saved artifacts belong to 1/10; keep their
  registered tests even when the most visible failing examples are Table users.
  Batch 10 also preserves the original named root Integer argument table in
  anchor configuration; bound executable types alone do not close that metadata
  obligation. Its owners are `AnchorArtifact.cpp` and the retained parameterized
  anchor cases in `test_stateful_static_parameters.py`.
- Multiclock domain isolation and existing physical scheduling/host sample
  capabilities belong to 10, with pending semantic choices below. No automatic
  multiclock scheduling or broader t-gfsim runtime is authorized here.

The unresolved semantic ledger must remain distinct from implementation gaps:

| Choice needing a decision | Old capability / affected owner | Bounded action |
| --- | --- | --- |
| X/Z guard ordering and selected failure diagnostics | Retained four-state control and source-check tests | Preserve original oracles; continue known-control and payload work; resolve reached-check ordering before changing unknown-control expectations. |
| Calling a Rule inside a transaction | Callable reuse fixtures and source admission | Keep explicit registration and Method nesting; decide whether Rule invocation exists and its atomic boundary. Do not recreate automatic registration. |
| Nonoverlap proof for field/Table writes | Table updates, pool and multiwriter tests | Implement demonstrably disjoint cases; record conservative rejection as an implementation limitation. Discuss actual ambiguous conflict semantics separately. |
| Memory timing not fixed by existing approvals | `pythonic-memory`, sync/async memory, bank/DMA histories | Inventory each latency/read-during-write/transactional effect against its existing specification; preserve established timing. Discuss only absent choices rather than declaring Memory cancelled. |
| Full-cycle System versus old sample/half-cycle/host actions | Historical host histories, reset/trace cases, multiclock isolation | Preserve the original histories and identify which require adapters or an explicit retirement decision; no silent cycle redefinition. |

Old-Q, explicit registration, old Queue occupancy and by-value Method arguments
are settled. Rewriting old forwarding-dependent expectations from those contracts
is implementation work. Cancelled Signal APIs and legacy candidate-state paths
remain retired, not unassigned preservation work. Long oracle/mutation/performance
and cross-platform release matrices retain their existing nightly/release scope;
this inventory neither runs them nor silently waives them.
