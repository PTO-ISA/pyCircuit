# Transactional frontend replacement status

PR-05 extends the production transactional Python compile/link path from
`5bfe4035` with Module control flow and autorules. This is a bounded replacement,
not completion of issue #361.

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
| Module if/elif/else and parameterless autorule | Implemented; retained SCF and focused C++/RTL execution PASS |
| Advanced payloads, Rule-body control flow and branch-local merging | Implementation gaps; retained tests/oracles |
| Memory/Table timing and advanced capability migration | Retained requirements; consult replacement plan for semantic decisions |
| Direct transactional gfsim | Later work; no alternate production frontend |

`hello_counter` explicitly registers `increment()`. Five cycles retain ten Work
observations: `0,0,1,1,2,2,3,3,4,4`. State assignment proposes an update; later
reads still read old Q. Local expressions carry newly computed SSA values.
Unregistered Rules do not execute. Disabled Rules publish no logs, and failed
global checks publish neither state nor pending observations.

## Module control and autorules

Module `if/elif/else` retains no-result `scf.if` and `scf.yield` in normalized
ACIR. `ac.source.predicate` associates each control with its original Bits value;
its i1 result does not prove the value known. Current execution admission rejects
controls that cannot be proved known; unknown payloads remain supported.

`@ac.autorule` normalizes to one ordinary Rule definition and registration.
It accepts no parameters/results and cannot be explicitly called again. Ordinary
Rules retain explicit registration. Definitions and allocations stay directly
in the Module; branch computations and read-only observations remain lexical.

Common analysis combines each registration path with its Rule guard. Else uses
the original condition, independent ifs have no priority, and all reads observe
old Q. The same activation controls state proposals, assertions and logs.
Mutual exclusion uses opposite path terms, conjunctions and equality of the same
actual State to distinct known constants. Overlapping writers still reject.
Routed/Final replay reconstructs paths from the retained source and checks their
actual data, enable, assertion and observation endpoints.

The new `module_control_state_machine` example starts with waiting state,
permit false and zero completions. The first cycle holds despite the independent
permit toggle. Its five old-state observations are `(0,0,0)`, `(0,1,0)`,
`(1,0,0)`, `(2,1,0)`, `(0,0,1)`, each sampled at low and high epochs.
A separate test writes one State from three equality-guarded autorules.

## Removal and verification accounting

The unused importer guard-to-`ac.branch` helper is removed; production Module
control stays SCF until common transactional analysis. The existing routing,
storage lowering, generated System runners and both backends are reused.
`return None` now denotes an empty return in both Rules and Systems.

The earlier automatic ordinary registration, write/binding-plan passes and
candidate-SSA orchestration remain removed. Shared AST, provenance, arithmetic,
source registry, publication and backend owners remain. Queue, hierarchy and
user Method execution are still gaps; no legacy fallback is restored.

Old examples and API cases remain migration requirements. Their current FAIL is
not a PASS. The retained `SourceMemoryBindingsTest.cpp` plan-class tests and
three entry-wrapper snapshot tests remain NOT RUN migration inputs. The old
TableQuery write-plan-internals case remains retired, not passing coverage.
No existing gate entry was removed; the state-machine example joins the gate.

Candidate commands, statuses, reviews and content hashes are recorded under
ignored `docs/gates/logs/pr05-scf-autorule/`. Public source artifacts and per-command
receipts remain in the existing lit scratch directory.

| Check | Actual result |
| --- | --- |
| Compiler tools build and installation | PASS |
| PR-01–04 transactional IR/state/failure regressions | PASS, 10 lit tests |
| Public source pipeline | PASS, C++ workers 1/2 and RTL; both state-machine forms, path suppression, failure hold, retained SCF roundtrip and polarity-tamper rejection |
| Capture/transport and example catalog/CMake Python tests | PASS, 81 tests |
| Standard hello_counter and state-machine examples | PASS, 2 CTest entries, generated C++ workers 1/2 and RTL |
| API gate | FAIL during build: module_loop requires Module inputs/hierarchy; remaining gate execution NOT RUN |
| Examples gate in fresh output directory | FAIL during build: multi_outputs inputs and queue_rule_pipeline Methods remain migration gaps; full gate execution NOT RUN |
| Changed-file pre-commit | PASS, CI hook selection, per-file invocation to avoid the local Black worker hang |
| mkdocs build --strict | FAIL, the same three pre-existing broken links in user-owned untracked replacement/design drafts |
| Nightly, full platform release, direct transactional gfsim | NOT RUN |

These results do not certify unexecuted legacy cases. No test oracle was derived
from DUT output, and no old capability was silently cancelled. No publication or
merge is included in this local delivery.
