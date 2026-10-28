# Source-unit workflow

pyCircuit captures ordinary Python hardware descriptions, resolves and lowers
hardware semantics in MLIR, and emits C++ and Verilog from the same verified
final artifact. There is one public compile/link/emit route. Capture never
executes design functions.

The [functional numeric provider example](../reference/functional-float-source-units.md)
shows reusable independent source functions, response Structs and explicit
provider closures. Its public build driver emits C++ and Verilog from each
same linked design; the generated boundary carries values and numerical effects
while an external consumer retains scheduling and architectural state ownership.

## Ownership through the compiler

The public commands separate source publication, linking and emission. Each
stage checks its input before publishing outputs; neither backend recaptures
Python or infers a different hardware meaning.

The separately versioned [transactional resource IR](transactional-resource-ir.md)
currently supports structural verification through `pycircuit-opt` only. Its
`unlinked`/`linked` stages are not source units or executable hardware; the
existing publication, link, saved-anchor and emit boundaries reject them.

| Stage | Existing owner | Responsibility |
| --- | --- | --- |
| Capture | `_source_capture.py`, `_source_transport.py` | Read a stable source snapshot, parse Python syntax and serialize the existing private AST transport. Design functions are never executed. |
| Compile and publish | `_source_compile.py`, `SourceUnit.cpp` | Invoke native source lowering, verify the resulting body/interface pair and atomically publish the unit, interfaces and dependency metadata. |
| Resolve and lower | `AnalyzeRuleWrites`, `InferSourceBindings`, `LowerPythonSource` | Resolve actual providers and bindings; infer hardware types, storage ownership, effects and rule dependencies; lower to existing common operations. |
| Inspect typed calls | `SourceMethodSummariesAnalysis`, `AnalyzeSourceMethods` | Check the complete Method call closure and recursion, then prove conservative total admission for reachable guards and effect-branch selections. These IR-only facts live in ACIRDialect; the pass lives in ACIRTransforms. |
| Normalize reused entries with parameterized state | `SourceEntryBoundary`, `EntryBoundary` | Establish one stateless bound root instance and independently prove its public interface, actuals and provenance before transaction planning. Preserve ordinary retained-definition checks and logical top identity. |
| Inspect transaction state | `SourceTransactionStateAnalysis`, `AnalyzeTransactionStateSSA` | Build a read-only plan of whole-Bits candidate data/write-intent pairs, bounded branch joins, scalar and admitted nominal local joins, invocation snapshots, source-path availability and ordered guard/selection controls. Link uses this before lowering packages containing source transaction operations. |
| Route transactions | `ac-route-transactions` | Normalize source calls/candidates into typed hierarchy routes, logical state/update pairs and sampled/effect metadata. Authenticate against original source, then independently verify the routed anchor before atomic replacement. |
| Refine storage | `ac-lower-transactions` | Convert verified logical state to physical storage; independently check SSA/pin correspondence and strict hardware final legality before atomic replacement. Hardware-only graphs retain their normal verification limits. |
| Simplify and describe | `SimplifyRecordWires`, `ExtractSourceInterface` | Simplify immutable record wiring and derive the public interface from verified hardware. Preserve nominal identity, dependencies and source ownership. |
| Verify published authority | `SourceHeaderRegistry`, `SourceBodySnapshots` | Check provider declarations, imports and body/interface agreement. An imported declaration does not acquire a caller's defaults or type authority. |
| Link | `_driver.py`, `SourceLink.cpp` | Consume the complete explicit unit closure, resolve imports to definitions, construct the selected root and verify the final hardware package. |
| Common analysis | `HardwareAnalysis` and its dependency, source-check and Work-partition owners | Share packed widths, nominal field layouts, dependency/effect checks and scheduling facts between targets. |
| Emit | `_emit.py`, `compiler/tools/emit.cpp`, existing target definition emitters | Consume the same verified final snapshot and realize C++ field planes or packed Verilog values. Retain source-owned output inventories and generated CMake metadata. |
| Execute | `pyc6_runtime`, `SystemRunner`, `SimSystem` | Work observes old state. Complete whole-system Precheck/Precommit before any root Xfer; failure discards proposals. |

The C++ and Verilog representations differ, but element order, field identity,
four-state value/known/Z information and clock/commit behavior must agree. For
example, admitting a Table-valued Struct field requires a matching declaration,
shared packed-layout analysis, source construction and projection, and transport
through both emitters. A verifier change alone does not establish support.

Verilog emission preserves the native root module name in
`generated.json` as `root_rtl_name`. External tools may use this identity instead
of guessing it from source function names. This metadata does not rename the
root or change internal identifier encoding. Older receipts remain readable for
managed replacement.

Both targets emit `ports.json` from the same verified root bindings and layout.
Schema version 1 records `target` and lists exported `inputs` and source `outputs` with logical name,
legalized `wire`, signature `ordinal`, resolved packed `width`, and packed leaf
`fields` (`path`, `low`, `width`). Outputs retain their `availability` companion
wire or null. `domains.clock/reset` identify input ordinals or null; the input
list includes compiler-generated physical domain ports. `controls` lists extra
public availability and, for managed Verilog only, lifecycle ports with direction,
role, wire and width. C++ does not advertise RTL lifecycle ports. Internal
defaulted/routed ports and `pyc_reset_active` are not external
ports. For a System, this describes the generated DUT wrapper, not the separate
zero-port simulation harness. The metadata does not itself select export policy
or assign external aliases.

Root integer parameters use `pycircuit link --parameters bindings.json`, where
the file is an array such as `[{"name":"W","value":"65"}]`. Values are canonical
signed decimal strings, not JSON numbers or expressions. Names must be unique
declared integer parameters. The compiler binds in declaration order; omitted
parameters retain provider defaults, including defaults referring to earlier
parameters. A required parameter without a binding fails. Duplicate JSON keys,
extra fields, unknown names and noncanonical decimals fail before publication.
The driver snapshots the input once and forwards only that snapshot to the
native linker; final IR records the selected actual bindings.

`pycircuit link --reset-semantics strict|if_one` selects one root policy in the
final artifact. Default `strict` rejects X/Z reset on a rising sample. `if_one`
activates reset only for known 1; 0/X/Z enter normal operation. Both targets,
all primitive reset inputs and all reset-masked source checks use this policy.
Unknown clock, effective enable and reached guard/effect selectors still fault.
No source unit, per-instance setting or emission option overrides the final policy.
Generated receipts and simulation results record `reset_semantics`; legacy final
artifacts without the attribute have strict semantics.

For ordinary single-domain register hierarchies, `emit --target verilog
--rtl-mode physical` selects physical-edge execution. The default remains
managed execution. Physical mode keeps data and compiler domain ports public;
checks aggregate internally before any edge commits. It preserves declared
closed startup values and synchronous reset, and reports
`PYCIRCUIT_PHYSICAL_FAULT` on invalid controls or clock transitions. Simulation
then freezes state; a physical reset does not clear the fault. Runners must reject
that diagnostic even if a testbench later prints success. This is not a host
Work/Discard interface. Residual non-guard checks, observations, non-register
resources, multiple domains, clock-data use and dynamic initializers require
managed execution. See [physical export](physical-rtl-export.md) for the precise
timing and verification policy.

Likewise, a pure region's admitted operations must be supported by both target
emitters. Nested Table construction and mapping inside an existing TableMap
use that operation's checked region and field-sensitive emission. TableMatch
retains its scalar predicate boundary until equivalent target support is
implemented and independently verified. Backend failures must not become a
fallback implementation or a second semantic path.

The [serial Rule/Method extension](../rfcs/serial-rule-method/README.md) has
a bounded source-to-typed-effects producer and shared native call/state analyses.
Source compilation retains explicit state reads/writes and Method calls.
Guarded nested Rules and standalone conditional observations enter this typed
route even without Method declarations or dummy state. The conditional-call/effect
and local nominal-join candidate is awaiting acceptance; this workflow describes
its scope without claiming fresh gate or execution results. Native sampled admission
verifies closed metadata, ordered path-sensitive admission prefixes, real D/E
and domain routes, private terminal uses, Collection limits,
and per-occurrence sampling plans. Source bodies/interfaces cannot supply the
routed/final-only attributes. The routing pass materializes these records for
all transactions, including literal true/false, state-free and read-only cases.
Original-source authentication checks captures, candidate versions, guard and
selection identities/locations, original branch paths, invocation bindings and
exact affected-definition inventories before standalone routed verification
checks the retained SSA, wiring and dependency closure. A separate refinement
proof checks logical-to-physical storage correspondence before strict final
verification. Each transition verifies a detached candidate before replacement;
mappings locate objects but do not supply semantic authority. The
[routed-anchor contract](../architecture/routed-anchor.md) defines these native
boundaries, stage markers, graph/executable distinction and retained snapshots.

C++ and managed RTL consume the same sampling plan, freeze physical pins and
ordered checks during Work/PREPARE, and commit pending history only after
whole-system success. False-fire, held and falling samples maintain legal
history; Discard and failure preserve committed history. Focused final-IR
execution has native workers 1/2 and four-state RTL evidence. The public dynamic
source regression additionally covers independent and cross-owner transactions,
candidate-at-entry guards, actual state/history rollback and native sticky
failure recovery. Separate generated Method `@system` tests exercise direct
body checks/observations, repeated calls, ordered identities and failed-epoch
publication protection. Module-root probes and prior checkpoints do not replace
that System evidence or certify later candidates.

Within one immutable check-plan invocation, `SampledAdmissionValidation` shares
complete definition-local boundaries and successful Rule prefix proofs across
catalog and wiring checks. Keys preserve exact definitions and full bindings;
each query still parses the actual Rule. Actual instance routes, physical domains,
private uses and each distinct root remain independently checked. Changed IR
requires a new context; reuse does not change verification limits or admission.

Source units can publish these typed operations and their declaration-only
interfaces. Callable-body assert/log/report, including statement-arm effects,
reuse the existing expect/observe
operations, original source occurrence/span and closed ordered summary variants.
They cannot supply final CheckID/ObservationID. Shared spec and provenance validators
serve definitions and interfaces; provider body rederivation remains authoritative.
State analysis now retains explicit body-check and observation records with their
original bound SSA values, invocation frames, source metadata and operand-derived
validity prefixes and source paths. Assertions create no guards or state versions.
The authored path must be proven true; MLIR derives the separate structural
execution path. False or unknown assertion conditions remain records.
The lowerer saves original statement-time operands, emits the six-field sampled
inventory and direct terminals, and authenticates their exact layout, identities,
metadata and event order against the original plan. The final verifier proves
final-fire/selected-arm paths, exact private terminal operands and actual
owner/domain routes. Both proofs precede replacement.

The final-only `ac.sampled_admission` attribute has exactly six fields:
`clock_capture`, `reset_capture`, `fire_result`, `controls`, `body_checks` and
`observations`. Tagged `controls` replace the old `guards` field: each entry has
`kind` (`callable_guard` or `branch_select`), `id`, `invocation`, `predecessor`
and `path`. Body checks and observations also carry `path`; a path is empty or
references an earlier selection and its arm polarity. Old shapes fail closed.
An unselected control bypasses admission; a selected guard requires known one,
whereas a reached effect selector requires known zero or known one. Effect
activation is final transaction fire AND its selected path. Raw state-write
intent keeps its four-state meaning and remains separate from admission.
See [conditional-effect contracts](../rfcs/serial-rule-method/conditional-effects.md)
for exact schemas, slots, private routes, budgets and acceptance obligations, and
[body-effect contracts](../rfcs/serial-rule-method/sampled-body-effects.md)
for the preceding straight-line slice.
Consumer compilation reads the interfaces, without loading provider
Python or Method bodies. Native final link lowers calls through ordinary
static instances with bounded whole-Bits branches and statement-arm calls/effects,
a shared actual clock/reset domain and unordered writers admitted only through
disjoint footprints or proven grant exclusivity. Ordinary results escaping an outer Rule require total admission over
reachable controls; statically unreachable controls do not prevent totality.
All arms still undergo structural/type/closure checks. A reachable effect
selector whose knownness is unproved requires an outer `-> None` Rule; this
does not prohibit using Method results within the transaction or exempt it from
sampling.
Final verification and emission reject any residual unlowered effects.

Cross-instance lowering preserves the original Module definitions and storage.
Definition-relative routing exports old Q and lexical SSA and carries candidate
D/E back through ordinary Instance ports. Shared final-only `ModulePortLayout`
validation distinguishes the original physical prefix from appended internal
ports. The selected root exposes only the original prefix and binds internal
inputs to known zero; an ordinary child binds the complete signature. Source
units/imports cannot supply this final-only authority. A definition needing new
ports cannot also be used by a Collection in this slice, and transaction writer
summaries cannot cross Collection; unaffected Collections keep their contracts.

The same Dialect analysis exposes a separately fallible direct transaction
summary and structural/body comparison APIs. They retain ordered effects,
source type constraints and direct instance references, with explicit unknown
dependencies. Body definitions and imported rows share one receiver resolver and
one call/instance closure and totality analysis. Registry admission validates
canonical providers and explicit dependencies; pair validation rederives every
published module and rejects missing provider bodies. See the
[summary contract](../rfcs/serial-rule-method/publication-summary.md) for fields,
scopes and authority obligations.

Receiver resolution shares fully verified declaration headers and direct-instance
inventories within one immutable analysis invocation. Each actual call still
checks its path, provider, parameter/type bindings and resource direction. The
independent lowering proof constructs its own receiver context and SSA identity
index from the original IR; it never accepts the producer's cached result as
proof. Contexts do not survive IR mutation or cross analysis/proof invocations.

Static Module reference formals publish their nominal callee and authored
argument position separately from physical data inputs. Instance bindings retain
canonical allocation occurrences or a forwarded reference formal. Calls through
that declared interface do not grant access to private descendants. Reference
semantics retain their existing schema marker and exact body/interface checks.
Unused reference formals still require this transaction interface contract.

Local autonomous registrations use their real FunctionDef occurrence; explicit
global-template bindings use their real Call occurrence. The private transparent
entry retains exact capture forwarding and public-result authentication. Source
ordering metadata is rejected rather than republished: neither native attributes
nor a loaded anchor can restore cross-Rule candidate forwarding.

A definition with unbound reference formals remains an authenticated source
template until an ordinary Module actually allocates it. Link validates all
supplied source contracts, including lexical reference authority, then excludes
only reference templates with no allocation path from any closed ordinary
Module definition. The input source units and their published interfaces remain
unchanged. Unused ordinary definitions retain their existing checks. Selecting
an unbound reference template itself as the link root is rejected.

Link resolves these bindings against actual allocation occurrences. Connected
transactions route at their common ordinary ancestor without cloning state or
storing a top-specific absolute path in a shared definition. The registration
identity includes its lexical allocation path. Public Rule results and their
whole-transaction availability return through compiler-owned input pairs to
their original consumers. Both source authentication and independent routed
verification check those connections. Source references and callable contracts
cannot remain as inert annotations in routed or final hardware.

Before transaction planning, link removes exact-type identity extracts and
pools equal literal constants used exclusively by same-block aggregate
constructors. Source units are authenticated before this normalization and are
not modified. Local pure helper completion uses the same predicates; imported
helper snapshots retain their published authority. Physical domain connections,
state initializers, direct observations and effectful operations are outside
this compaction boundary.

Native module imports require an explicit `semantic_kind`. Hardware imports
carry `dependency_summary`; transaction imports carry `transaction_summary`.
The two summaries are mutually exclusive, and transaction imports cannot claim
primitive behavior. Untagged imports are rejected. Hardware dependency analysis
rejects transaction imports throughout the actual instance closure, including
result-free children; final package checking also rejects unused transaction
imports. Publication alone does not establish hardware execution.

FIFO Modules use canonical hardware imports and ordinary `ac.instance`
allocations. Their native `push`/`pop`/`peek` contracts terminate ordinary Method
resolution without an invented body or invocation frame. Published instance rows
retain logical payload and raw-direction binding facts; configuration is derived
from the canonical callee and ordinary parameters. The existing Queue access
plan, source replay and final wiring proof use that same actual instance identity.
Final hardware contains native FIFO instances, not dedicated Queue operations.
Both emitters continue to use the existing FIFO kernels.

Source compilation defers the hardware-only record simplifier for packages
retaining transaction semantics. The existing `--source-import-out` diagnostic
still denotes lowered hardware and rejects these packages; published source
bodies retain their actual typed IR. Link checks complete source-unit authority,
then applies the common transaction lowerer. It rechecks canonical primitive
imports with the admitted registry; native transformation separately validates
their hardware schema. Each admitted scalar state becomes one compiler-selected
DFF or DFFE with original clock/reset/init; Table states remain element
Collections. Candidate D values come from isolated pure Rules. All
staged hardware checks succeed before the package becomes final.

The explicit `pycircuit-opt --ac-analyze-transaction-state-ssa` pass preserves
its input IR. Every independent outer transaction starts from old Q. Registration
order does not expose another transaction's candidates, and `after` metadata is
rejected. Nested Method and source-global template calls
share that transaction's candidate versions while keeping ordinary SSA values
as snapshots. A bounded branch forks the same incoming data/raw-intent pair into
both arms and joins both components with the original four-state selector.
Missing writes inherit the branch-entry pair; identity writes remain in the
writer inventory. Branch results admit Bits, nominal Enum and finite acyclic
Structs with Bits/Enum/Struct fields. Changed nominal arms require identical
nominal types; unchanged Table-containing captures retain their existing bypass.
Local Attribute-only field assignments reconstruct ordinary Struct snapshots,
including subtrees, without changing saved aliases or persistent owners. The
root retains its nominal binding; field replacements use declared field types.
Table-containing roots and indexed/slice/annotated/augmented targets reject.
Common lowering projects fields, selects Bits leaves and Enum raw carriers, then
reconstructs Structs. Enum reconstruction leaves its generated membership result
unused, preserving invalid/X/Z payloads without a new fault. Source replay
authenticates the complete ordered graph and every intermediate before the
independent final proof; the seven-field branch summary and its three-field
result rows are unchanged. The native analysis requires actual Method bodies,
proven-true authored effect paths and paired clock/reset wire identity. It tracks source
paths and ordered guard/selection prefixes. A Method result becomes available at
callee exit on its defining path even if its payload aliases a literal, capture
or earlier value. Only verified branch joins lift arm values to the parent path.
False or unknown guards remain explicit obligations for sampled lowering and
checks. Calls/effects make all enclosing selectors require knownness; pure
branches retain four-state joins. Unsupported predicate types, calls/effects in
pure-expression positions, effectful loops, early returns, partial or aggregate
state, changed Table-containing local joins, resources, multiple outer writers and
unproved domains reject this analysis, including in constant-false arms. No
plan is published if any transaction fails.

New nominal closure/metadata traversal, structural-container allocation,
materialization and proof costs are reserved before use under existing limits.
The inherited arbitrary-precision arithmetic evaluator capacity policy remains
OPEN; these debits do not establish full evaluator bounds or close
`METHOD-ANALYSIS-BOUNDS`.

The analysis alone is not execution evidence; the bounded lowerer's public
C++/RTL execution and the broader extension have separate acceptance scopes.

For exact stage evidence, the native linker accepts paired
`--transaction-input-out PATH` and `--transaction-output-out PATH` in final-link
mode. It captures the actual merged input after preanalysis and the same
lowering pass's verified output. Both paths must be fresh ordinary files,
distinct from inputs, final output and owner report. Failure removes files
created by that invocation. The optional library `SourceLinkStages` output is
updated only on full success. These diagnostics add no Python public CLI or
source authority. Replaying the same body/header bytes, SourceOwner order, top
and linker must reproduce the public final byte-for-byte; the after-stage
snapshot equals that final serialization. Both emitters still use the public
final artifact.

Importer-local signatures are ordinary declaration attributes. They are not
temporary published imports and do not acquire dependency authority. The
completed real module is still verified before replacing the captured source
or publishing outputs.

Record simplification preserves the declared result type of every projection.
If an equivalent aggregate carries different source provenance and no exactly
typed replacement exists, the pass retains the verified projection. Fixed-bit
identity extracts keep their existing provenance handling; aggregate types are
not rewritten merely to enable an optimization.

C++ Work emission can materialize a proven constant Table field plane once in
a capture-free, function-local `static const` initializer. The existing emitter
constructs its exact type and layout; it does not introduce another constant
interpreter or change the final IR. Eligibility follows individual fields through
literal construction, projection and supported fixed conversions. Inputs, state,
region arguments and other computations retain ordinary emission. A dynamic row
keeps the entire selected field plane dynamic. Reset and Verilog use their
existing paths, and unknown indices retain their four-state behavior.

The immutable object is shared only within its generated static site and C++
template specialization. Its host lifetime extends to process exit; generated
code measurements must include first-use initialization and resident memory,
as well as repeated simulation time. This representation adds no hardware state
or source API.

## Compile and link

Compile each source independently. Consumers use published interfaces through
`-I`; link receives the complete explicit unit closure. For a two-source design:

```bash
mkdir -p .pycircuit_out/units
pycircuit compile -c src/child.py --source-root src \
  --package-prefix demo -o .pycircuit_out/units/child
pycircuit compile -c src/top.py --source-root src \
  --package-prefix demo -I .pycircuit_out/units/child \
  -o .pycircuit_out/units/top
pycircuit link .pycircuit_out/units/child .pycircuit_out/units/top \
  --top demo.top.Top -o .pycircuit_out/design_top.ac
pycircuit emit .pycircuit_out/design_top.ac --target cpp \
  -o .pycircuit_out/cpp
pycircuit emit .pycircuit_out/design_top.ac --target verilog \
  -o .pycircuit_out/verilog
```

To preserve the routed boundary before physical refinement, use the same explicit
unit closure with `--target anchor -o ANCHOR_DIR`. The resulting
[saved anchor](../architecture/saved-anchor.md) contains a versioned graph,
separate exact definition specs/bodies and retained source interfaces. After the
original sources and units are removed, `pycircuit link --from-anchor ANCHOR_DIR
--verify-only` checks it without routing or refinement. `pycircuit link
--from-anchor ANCHOR_DIR --target final -o FINAL.ac` explicitly refines that
verified graph. Reload cannot override its root, parameters or reset policy.
It reuses the existing publisher for outputs and accepts portable artifact-only
copies through a bounded stable snapshot.

Generated CMake builds source-owned `pycircuit_modules` against the Runtime
component. Host drivers use the typed `pyc_dut` and shared SystemRunner, supply
inputs and explicit clock/reset levels, and set a finite run limit. Work reads
old state; successful whole-system checking precedes Xfer commit.

## Scope and verification

The [language reference](../reference/language.md) defines current admission and
unsupported constructs. The [frontend guide](agent-frontend-guide.md) describes
source authoring. CompilerDev requires exact LLVM/MLIR 22.1.8; Runtime-only
consumers use `find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)` and
`pycircuit::pyc6_runtime` without LLVM discovery.

The current example catalog includes retained supported designs. Incomplete
historical examples and API drafts were removed at the user's delivery cutoff
on 2026-10-07; removal does not establish their implementation or verification.
Historical planning and logs remain local ignored records and in Git history.
They are not dependencies of examples, API tests or documentation builds.

Use [testing and gates](testing-and-gates.md) for short validation and nightly
coverage. Preserve source-import IR, transformed IR and both backend outputs
through existing manifests when running compiler validation. Long oracle,
coverage and platform matrices are scheduled separately; omitted runs must be
reported as not run.

For retained gate evidence, the native source-unit harness accepts an optional
`--source-import-out PATH`. It saves the complete hardware IR after source
lowering and before record-wire simplification from that same successful
pipeline run. The destination must be fresh; existing files and aliases to
inputs or other outputs are rejected. A failed compilation or output write does
not keep this invocation's diagnostic file. Ordinary native outputs retain
their sequential write behavior; this is not a multi-file transaction.

Keep this diagnostic file outside the published source-unit directory. It
carries no interface or provider authority. When generated through a native reproduction
for evidence, bind its capture, headers, owner, tool and command, and compare
the reproduced body/interface with the public compilation before associating
its stages. The public compile/link/emit route is unchanged.

## Stateless numeric provider composition

The [functional numeric examples](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/README.md)
place typed sequential intermediates in ordinary functions and expose a thin
`@ac.module` entry that returns the complete result/status response. For
example, `float_round_integral.py` composes independently published integer
rounding and integer-to-float function bodies. Compile the provider interfaces
first and pass the complete body closure to link; emit both targets from that
same final artifact. The example driver records this route explicitly.

An annotated variable declared directly in a `@ac.module` body is a
module-owned variable and requires its specified static initializer. Use an
ordinary function's typed SSA intermediates for stateless numeric calculation;
a dynamic module-variable initializer is not a temporary-value workaround.
Result availability and numeric flags remain part of each declared algorithm
response. An ISA or architectural consumer owns source lifetime, type/layout
admission, state commit and memory effects rather than encoding those concerns
into the public arithmetic provider.

## Retired routes

CycleAwareSignal/JIT, structural builders, Agentic Circuit/QueueGraph, `acc.py`,
`acc`, and `pycc` are retired. They are not aliases or fallback implementations.

## Reusable computation with a stateful consumer

The [functional word-state example](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/README.md#composing-a-response-with-owned-state)
shows an imported numeric source unit feeding one Rule over module-owned
variables. The result is a typed snapshot of committed value/count/status;
it is not the speculative candidate of the current Work. Keep the response's
request-domain policy explicit: the example's inactive request holds all state,
while an active rejected word request only records its sticky rejection. IEEE
invalid numerical outcomes and unknown/unavailable wires remain different
contracts. A valid/invalid wrapper must not turn an unavailable computation
into zero or discard a defined floating NaN/status outcome.

Both target artifacts come from the verified linked closure. The registered
System example uses the common generated runner, complete checks before Xfer,
and strict sampled domains; authors do not tie off commit controls or insert
clock/reset pins in their computational model. The eight-snapshot native1/2
and RTL check is public and independently reproducible with the installed SDK.
It does not require the private SSM adapter or certify unrelated runtime gates.
