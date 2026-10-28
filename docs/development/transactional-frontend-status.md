# Transactional frontend replacement status

PR-06b extends PR-06a (`e1e91017`) with independently compiled Module reference
bindings and atomic sibling Method calls. This is a bounded replacement,
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
| Local Value/Action/ActionValue Methods and nested calls | Implemented; see PR-06a validation below |
| Existing-instance references and sibling Method calls | Implemented; see PR-06b validation below |
| Module/Rule resource interfaces | PR-07 candidate; State execution and Queue structural validation recorded below |
| Bits Queue execution | PR-08 candidate; validation and remaining scope recorded below |
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
source registry, publication and backend owners remain. Queue and broader resource interfaces remain gaps; no legacy fallback is restored.

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

## Local Method admission (PR-06a)

The native importer now retains local `ac.method` definitions with independent
guard/body regions and `ac.method.call` operations. Common analysis binds each
invocation's original arguments and owner captures, expands its old-Q computation,
and collects all transitive guard requirements. Routing and independent replay
prepare payloads before applying the final outer Rule fire to every effect.
No call-order arbitration, parameter writeback or state forwarding is added.

The `local_method_accumulator` example covers Value, Action and ActionValue calls,
read-only Methods in guards, nested calls, distinct arguments and returned new
SSA values alongside old state reads. Its late guard cancels earlier writes and
logs while an independent autorule toggles. Focused acceptance also checks one
multiwrite rejection and global assertion rollback. Source ACIR is roundtripped
with definitions and calls intact. Queue, advanced types and same-cycle dependency
migration remain outstanding; their retained tests/oracles are not removed.

PR-06a evidence is recorded in `docs/gates/logs/pr06a-methods/`; the preceding
PR-05 table is historical evidence only. Final command outcomes are recorded in
that candidate's receipt, including unsuccessful first attempts. Strict MkDocs
still reports the three broken links in the pre-existing untracked design drafts.

| PR-06a check | Actual result |
| --- | --- |
| Compiler build and installation | PASS |
| Public Method acceptance | PASS: C++ workers 1/2, RTL System execution, direct C++/Icarus assertion rollback, nested guard refusal, multiwrite rejection |
| Prior transactional IR/state/source checks | PASS: 11 lit tests |
| Changed-file pre-commit | PASS: CI hook selection; Python files run individually due local multi-file Black hang |
| API gate | FAIL at unported `module_loop` Module inputs; remaining execution NOT RUN |
| Examples gate with rebuilt installation and fresh output | FAIL at unported `multi_outputs` inputs and `queue_rule_pipeline` body syntax; complete execution NOT RUN |
| Strict MkDocs | FAIL: three pre-existing broken links in untracked design drafts |
| Nightly, release matrix and direct transactional gfsim | NOT RUN |

## Existing sibling references (PR-06b)

`Sender(peer: ac.module)` compiles independently with inferred Method requirements.
Published provider interfaces carry actual Method signatures and verified read-only
properties. Link validates each actual binding against its complete body and
specializes by instance occurrence. It does not create an instance for a reference.
Method arguments remain SSA values; callee captures use the actual callee's
prepared combination and old State, independent of Work ordering.

Common analysis records instance/resource owners and Method invocation paths.
The same final Rule admission controls all owners' proposals, checks and logs.
Routing preserves actual source children and their State storage. Compiler ports
carry old Q upward and proposal data/enables downward; each owner retains a
closed initializer expression. Common
transaction computations are statically specialized at the root. This is not a
stable public Method ABI. Independent Routed/Final replay checks those actual
hierarchy edges as well as the transaction computation and effect enables.

The split-source sibling example creates two Receivers initialized to five and
binds Sender to only one. Independent expected old values are `5,6,7,7`, the unused
Receiver stays at five, and Sender successes are `0,1,2,2`. Its prior log and update
are cancelled when the callee guard closes. Acceptance checks interface-only
consumer compilation, explicit link mismatch rejection, physical state ownership,
source observation identity and generated C++ workers 1/2 versus RTL.

The minimal reference-binding and cross-owner State pieces previously assigned to
PR-09/10 are delivered here. Those later packages retain cross-layer exports,
resource aliases, Queue and batch operations; they reuse this analysis and link
binding instead of implementing another path.
The original PR-06b signature inference described here is superseded by the
SSA/Method constraint repair below. Queue, advanced payloads and same-cycle dependencies retain their
existing tests and independent oracles as unported requirements.

PR-06b candidate evidence is kept in `docs/gates/logs/pr06b-sibling-methods/`.

| PR-06b check | Actual result |
| --- | --- |
| Compiler tools build and installation | PASS |
| Split-source sibling acceptance | PASS: C++ workers 1/2 and RTL, strict link mismatch rejection, callee combination capture, physical state owners and distinct observation identities |
| Prior transactional IR/state/source/local Method regressions | PASS: 12 lit tests; an earlier concurrent scratch collision was discarded and rerun |
| New installed-toolchain examples | PASS: both CTest entries, generated C++ workers 1/2 and RTL |
| Transitive generic read-only requirement | PASS: independent compile smoke through a local guard helper |
| Independent implementation and oracle reviews | Clear, exact content hashes retained in evidence |
| Changed-file pre-commit | PASS with CI hook selection and individual Python file invocations |
| Full API and Examples gates | FAIL at retained unported Module data/resource inputs and Queue body syntax; complete gate execution NOT RUN |
| Strict MkDocs | FAIL: same three pre-existing draft links |
| Nightly, release matrix and direct transactional gfsim | NOT RUN |

## Parent resource interfaces (PR-07)

The candidate carries Module/Rule payload constraints as primitive resource
interfaces, specializes actual instance bindings at link, and retains existing
State/Queue identities through exports. Bound outputs use parent resources;
new connections default to a parent-owned depth-2 Queue. Queue execution remains
rejected. Rule input payload reuse does not request repeated Queue pops.

`parent_state_interfaces` uses separately compiled global Rules, Stage and Export
Modules, and a parent with distinct State bindings. Its independent trace checks
old Q and joint output/internal-state hold. Generated-model adapters exercise
reset separately. `sibling_calls` retains Writer/Reader and its `5,6,7,7` oracle.

Candidate evidence is recorded under
`docs/gates/logs/pr07-resource-interfaces/`.

| Check | Actual result |
| --- | --- |
| Compiler tools build and installation | PASS using narrow tool targets and direct install |
| Parent State execution and reset | PASS, C++ workers 1/2 and RTL with independent traces |
| Resource structural linking | PASS, State/Queue specialization, parent depth 2/5 allocation, alias identity, repeated-input single pop and print/reread |
| Migrated sibling_calls | PASS, full Writer/Reader and original oracle |
| Four previously runnable examples | PASS, generated C++ workers 1/2 and RTL |
| Transactional IR regressions | PASS, 8 selected tests |
| Prior transactional execution regressions | PASS, 4 tests with stable tool hashes |
| Mismatched binding and publication protection | PASS, rejection preserves published unit bytes |
| Changed-file CI hooks | PASS, including Ruff and Black |
| Capture, transport and example infrastructure | PASS, 167 tests |
| API gate | FAIL during build of retained historical module_loop; tests beyond build NOT RUN |
| Examples gate | FAIL during multi_outputs compile: unsupported expression; selected runnable examples checked separately |
| Strict documentation build | FAIL, three preexisting broken links in local replacement-plan/RFC files |
| Full nightly | NOT RUN |

An initial example run encountered an incompatible legacy published unit in the
shared output directory; the runnable example selection was regenerated and
passed in a fresh directory. No old artifact was used as current evidence.
Queue execution remains deferred to PR-08; structural PASS is not simulation.

## SSA, Method and resource constraint repair

Local type propagation now uses SSA constraints in common ACIR analysis, replacing
fixed AST scans and name-wide hints. Exact literal values, arithmetic relations
and boundary conversions remain distinct. Method requirements aggregate ignored,
scalar and tuple uses; ignored values preserve unknown return arity until actual
provider binding. Source contract 3 publishes these requirements and requires
regeneration of older source units.

Resource connections now survive capture as IR. Link checks complete aliases,
exports, payloads and allocation identities before selecting one existing resource
or allocating one parent-owned depth-2 Queue. The importer no longer allocates
default Queues or scans assignments backward to infer output aliases.

The short CI native job builds the toolchain and exercises constraint/resource
native tests and transactional C++ workers 1/2 plus RTL regressions. Local evidence
for this candidate is recorded under `docs/gates/logs/ir-constraint-repair/`.
Actual outcomes are recorded after verification; earlier tables are historical.
Queue execution, complex control flow and the retained historical gate gaps stay
open. No PR publication or merge is implied by this local repair.

| Constraint repair check | Actual result |
| --- | --- |
| Native compiler build and installation | PASS |
| Native Method/resource tests | PASS, 2 CTest entries containing 3 tests |
| Transactional IR and prior execution regressions | PASS, 14 lit tests |
| New constraint execution | PASS, C++ workers 1/2 and RTL; long aliases, tuple scalar calls, captured deep expressions, fixed-width overflow and ignored ActionValue effects |
| Independent implementation review | Clear for A/B and C in separate instances; exact hashes in local evidence |
| Changed-file CI hooks and repository policy validation | PASS, including Ruff and Black on changed Python files |
| Capture/publication and metadata Python tests | Initial run: 361 PASS, 3 SKIP, 1 FAIL in unchanged snapshot-race test; isolated rerun PASS |
| Full API and Examples gates | FAIL during retained historical module_loop and multi_outputs compilation; execution beyond build NOT RUN |
| Strict documentation build | FAIL, the three preexisting links in user-owned local design drafts |
| GitHub CI, full nightly and direct transactional gfsim | NOT RUN |

The sibling rejection now tests actual narrowing (u8 argument to u4 parameter);
u8-to-u16 widening is legal. The previous Queue execution-rejection assertion
was removed because unsupported execution is a migration gap, not a permanent
contract. Queue structural identity/depth checks remain; removed execution
coverage is not recorded as passing.

Earlier repair attempts failed on pending-type admission, tuple demand,
conversion propagation and deep captured arithmetic; those failures and their
later passing independent oracles are retained in the local receipt. One
regression attempt overlapped tool rebuilding and failed while its executable
was replaced; the stable-tool run above passed all 14 tests. The short native
job builds explicit compiler/runtime/test targets, preserving the full API gate
and its existing historical failures rather than dropping those entries.

## Bits Queue execution (PR-08)

This local candidate depends on the uncommitted PR-07 resource/constraint
candidate at `0db42d90` plus its saved patch and untracked sources. The exact
baseline, candidate content, separate PR-08 delta and raw evidence are retained
under `docs/gates/logs/20261028-pr08/`; no remote PR or merge is claimed.

Common analysis now models Bits Queue allocations, actual owner/depth, old head,
read/write admission and separate consume/enqueue proposals. Rule interfaces and
explicit resource Methods use that same plan. Module `peek()`/`head()` observations
propagate nonempty requirements to dependent transactions. Reused SSA payloads do
not consume twice. Same-direction accesses require proven exclusivity; preserving
whole Boolean facts alongside decomposed facts fixes complementary composite
Module paths without adding arbitration or a general Boolean solver.

The adapter uses existing `fifo_local` storage with latency one and old-occupancy
readiness. FIFO storage remains in its source owner. Both targets consume the same
verified Final artifact; replay checks FIFO configuration, ownership, status,
payload and final-fire proposal wiring. Whole-system checks precede commit and
publication. No retired Queue operations or alternate compilation path were added.

`scalar_queue_pipeline` connects BiasStage, SumStage and an existing Accumulator.
The latter returns its explicitly computed new SSA total; ordinary state reads
remain old Q. Default depth-2 connections and an explicit depth-3 connection carry
3, 5, 2, 7 to the independently expected 4, 10, 13, 21 sequence. A delayed consumer
checks backpressure against an independent old-occupancy model.

| Check | Actual result |
| --- | --- |
| PR-07 constraint/resource native tests and constraint/State-interface lit | PASS on fixed baseline |
| Current candidate native tests and transactional IR/CodeGen regressions | PASS, 2 native tests and 15 lit tests |
| Queue pipeline default/depth-3/later Method guard | PASS, C++ workers 1/2 and Verilator |
| Empty/full/intermediate occupancy, same-Queue pop+push and reset | PASS, C++ workers 1/2 and Icarus |
| Failed assertion, discarded retry, Queue contents and State hold | PASS, C++ workers 1/2 and Icarus |
| Child-owned Queue, nonconsuming observation and failed-epoch log suppression | PASS, generated C++ workers 1/2 and RTL Systems |
| Mutually exclusive shared accesses / duplicate explicit pop | PASS, execution / rejection respectively |
| X/Z payload fidelity | PASS, Icarus; payload does not control admission |
| Saved linked/Routed/Final reread and capacity/pop-edge tampering | PASS, valid replay and invalid correspondence rejection |
| Seven currently runnable registered examples including the new pipeline | PASS, installed System verification |
| Full build and API gate | FAIL at retained `module_loop` unsupported callable; complete gate execution NOT RUN |
| Full Examples gate in a fresh output directory | FAIL at retained `multi_outputs` unsupported expression; complete gate execution NOT RUN |
| Strict documentation build | FAIL on three pre-existing links in the PR-07 plan/design documents |
| Nightly and all-platform release | NOT RUN; outside this bounded slice |

The failed documentation links are the replacement plan's repository-local issue
link and the transactional design's missing migration-status/gfsim-plan links.
Those predecessor documents remain unchanged in this dependent delta. An initial
Examples attempt also rejected stale source-contract artifacts; the fresh run
above is the relevant candidate result. Changed-file Ruff, Black and remaining pre-commit hooks pass, as does the API
hygiene check. Independent review is clear for the candidate; these results do
not replace the failed full gates or authorize merge. Exact review and command
records accompany the candidate evidence.

The original `queue_rule_pipeline` and its oracle remain unchanged and pending
Struct, loop and old write/read migration; this scalar example is not its recovery.
Advanced payloads, Rule-body control flow and direct transactional gfsim remain
separate work. Root Queue export to an external payload-only interface is rejected
rather than exposing an unavailable head as a successful read; nested resource
exports retain identity and are exercised here.

## Multilevel references and independent outputs (PR-09)

PR-09 builds on the exact local PR-08 candidate at `0db42d90` plus its existing
uncommitted sources. Baseline bytes/hashes, the separate PR-09 delta and command
receipts are retained in `docs/gates/logs/20261028-pr09/`. No compiler change was
needed: existing SSA constraints, resource connection resolution, per-instance
specialization and verified routing already compose these Bits capabilities.

The five-unit `resource_composition` example passes an existing Counter through
Relay to SplitStage and reexports its two independent Queue outputs. One Rule
consumes 1–4, calls the original Counter and returns input plus 10 and plus 20.
The delayed right consumer fills its Queue; input consumption, both enqueues,
local accepted count and remote Counter update then hold together. The other
Counter remains zero. Independent old-occupancy models check every low/high
epoch, output ordering and completion, rather than deriving expectations from
the generated design.

Focused companion cases cover two independently admitted Rules, the same tuple
template bound to State/Queue and Queue/Queue, old-State observations through
multiple export aliases, and rejection of two output positions writing one
aliased Queue. Linked allocations retain their owners and unique identities;
Routed/Final replay and FIFO counts check storage preservation. Every example
source compiles separately using provider interfaces and links an explicit
closure after the Python sources have been removed from the test scratch tree.

This covers the Bits combination portions of PR-09–11, reusing the cross-owner
atomicity and rollback covered by PR-06b/08. It does not restore the historical
`multi_outputs` combination/Struct interfaces or the sequential candidate-state
forwarding of `sibling_aliases`. Their sources and independent oracles remain
migration inputs. Struct/Enum payloads, batch control flow and root external
Queue protocols remain separate work; no old test was retired in this slice.

| PR-09 check | Actual result |
| --- | --- |
| Compiler targets and native Method/resource tests | PASS, unchanged compiler; 2 CTest entries |
| Existing transactional regressions | PASS, 17 lit tests |
| Resource composition acceptance | PASS, 7 RUNs; three scenarios on C++ workers 1/2 and RTL, plus aliased double-write rejection |
| Seven preceding runnable examples plus resource_composition | PASS, 8 CTest System entries, generated C++ workers 1/2 and RTL |
| Example catalog/CMake/verifier unit tests | PASS, 118 tests |
| Changed-file CI hooks and API hygiene | PASS; Python files checked individually, including Ruff and Black |
| Independent authoring and implementation reviews | Clear; candidate hashes in local evidence |
| Complete API / examples gates | FAIL at retained module_loop / multi_outputs migration gaps; execution beyond build NOT RUN |
| Selected aggregate CMake build | FAIL: parent_state_interfaces predicts rules.cpp although its global-Rule unit emits a header; independent CTest System flow above passes |
| Strict documentation build | FAIL, three existing replacement-plan/RFC links; no new broken link retained |
| Nightly, release, remote CI and direct transactional gfsim | NOT RUN |
