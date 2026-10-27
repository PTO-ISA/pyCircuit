# Transactional frontend replacement status

PR-04 replaces the production Python compile/link path after PR-01–03
(base `7d0c79b9`). This is a bounded replacement, not completion of issue #361.

## Production path

Python capture remains syntax transport. Native source lowering publishes
transactional v2 unlinked resource bodies and interfaces. The existing registry
checks source owners, imports, signatures and exact declaration snapshots.
Consumers compile against interfaces; link requires the complete explicit body
closure. Old or mixed semantic units require regeneration. Publication retains
its existing locks, snapshots, dependency files and failed-output protection.

Link authenticates the closure, materializes constants, and routes linked ACIR
through common transactional analysis and independently checked Routed contract
4 before existing storage lowering. C++ and Verilog consume the same Final
artifact. The source-import diagnostic now retains normalized unlinked ACIR;
transaction input/output diagnostics retain linked and Final ACIR respectively.
Routed artifacts remain available through the existing Routed link target.

## Current slice and retained requirements

| Capability | Migration status |
| --- | --- |
| Closed Module and default single-clock System | Implemented; focused source execution PASS |
| Explicitly initialized Bits State and State exports | Implemented; focused source execution PASS |
| Pure expressions, nested Rule definition and explicit call | Implemented; focused source execution PASS |
| Guard, assert, log and old-Q reads | Implemented; focused source execution PASS |
| Literal constant providers through published interfaces | Implemented; focused source execution PASS |
| Queue, hierarchy and user Method execution | Implementation gaps; retained tests/oracles |
| autorule, advanced payloads and control flow | Implementation gaps; retained tests/oracles |
| Memory/Table timing and advanced capability migration | Retained requirements; consult replacement plan for semantic decisions |
| Direct transactional gfsim | Later work; no alternate production frontend |

`hello_counter` explicitly registers `increment()`. Five cycles retain ten Work
observations: `0,0,1,1,2,2,3,3,4,4`. State assignment proposes an update; later
reads still read old Q. Local expressions carry newly computed SSA values.
Unregistered Rules do not execute. Disabled Rules publish no logs, and failed
global checks publish neither state nor pending observations.

## Removal and verification accounting

The former native importer, automatic registration, write/binding-plan passes,
and production link candidate-SSA orchestration are removed. Shared AST,
provenance, arithmetic, source registry, publication and storage/backend owners
remain. The prototype contributed lowering logic, not its profile, CLI or
physical bypass.

Old examples and API cases are retained as migration requirements. Their current
FAIL is not a PASS, and their unsupported syntax is distinct from an installation
or compiler defect. The direct `SourceMemoryBindingsTest.cpp` plan-class tests
remain as deferred migration input outside the compiled target because their
implementation classes were deleted; their coverage is **NOT RUN**, not passed.
Three entry-wrapper snapshot tests remain as NOT RUN migration inputs because
their old stage fields were removed. The single TableQuery test of retired
write-plan internals is removed. Remaining
source transaction tests invoke the replacement compile entry and retain their
original oracles. Existing gate cases remain selected; the new transactional source test is added
to the API gate.

Candidate command logs and source/generated artifacts are retained under ignored
`docs/gates/logs/pr04-production-frontend/` and the existing lit scratch directory.
Current candidate outcomes:

| Check | Actual result |
| --- | --- |
| Compiler tools build and installation | PASS |
| PR-01–03 transactional lit selection | PASS, 10 tests |
| Independent public source pipeline | PASS, C++ and RTL Systems; C++ State-export Module |
| hello_counter generated System | PASS, workers 1/2 and RTL, 5 cycles / 10 epochs / original logs |
| API gate | FAIL during build: module_loop requires Module inputs/hierarchy outside this slice; remaining gate execution NOT RUN |
| Examples gate in a fresh output directory | FAIL: multi_outputs inputs, queue_rule_pipeline Methods, rob Struct defaults remain migration gaps |
| Changed-file pre-commit | PASS, exact CI hook selection (separate file batches to avoid local Black worker hang) |
| mkdocs build --strict | FAIL: three broken links in pre-existing untracked replacement-plan/design drafts |
| Nightly, full platform release, direct gfsim | NOT RUN |

An initial examples attempt rejected pre-existing old source artifacts before
replacement. A fresh output directory removed that cache-version ambiguity;
the failures above are current-source capability gaps. Regeneration requires a
fresh unit destination when an old artifact cannot pass current authentication.
The failed docs links are in `transactional-frontend-replacement-plan.md` and
`transactional-acir-design.md`, not the new status links. User draft files remain
untouched. No gate inventory was reduced to obtain these results.
