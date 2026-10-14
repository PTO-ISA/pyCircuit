# Source-unit workflow

pyCircuit captures ordinary Python hardware descriptions, resolves and lowers
hardware semantics in MLIR, and emits C++ and Verilog from the same verified
final artifact. There is one public compile/link/emit route. Capture never
executes design functions.

## Ownership through the compiler

The public commands separate source publication, linking and emission. Each
stage checks its input before publishing outputs; neither backend recaptures
Python or infers a different hardware meaning.

| Stage | Existing owner | Responsibility |
| --- | --- | --- |
| Capture | `_source_capture.py`, `_source_transport.py` | Read a stable source snapshot, parse Python syntax and serialize the existing private AST transport. Design functions are never executed. |
| Compile and publish | `_source_compile.py`, `SourceUnit.cpp` | Invoke native source lowering, verify the resulting body/interface pair and atomically publish the unit, interfaces and dependency metadata. |
| Resolve and lower | `AnalyzeRuleWrites`, `InferSourceBindings`, `LowerPythonSource` | Resolve actual providers and bindings; infer hardware types, storage ownership, effects and rule dependencies; lower to existing common operations. |
| Inspect typed calls | `SourceMethodSummariesAnalysis`, `AnalyzeSourceMethods` | Check intermediate Method call closure, recursion and conservative guard totality. These IR-only facts live in ACIRDialect; the pass lives in ACIRTransforms. |
| Inspect transaction state | `SourceTransactionStateAnalysis`, `AnalyzeTransactionStateSSA` | Build a read-only plan of straight-line whole-owner candidate versions, invocation snapshots and guard obligations. Link uses this before lowering packages containing source transaction operations. |
| Lower transactions | `LowerTransactions` | Materialize straight-line, same-domain whole-Bits transactions with sampled metadata and complete wiring records. Authenticate against the original plan, then run canonical final verification before atomic replacement. Ordinary hardware retains existing verification without transaction-analysis limits. |
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

Likewise, a pure region's admitted operations must be supported by both target
emitters. Nested Table construction and mapping inside an existing TableMap
use that operation's checked region and field-sensitive emission. TableMatch
retains its scalar predicate boundary until equivalent target support is
implemented and independently verified. Backend failures must not become a
fallback implementation or a second semantic path.

The [serial Rule/Method extension](../rfcs/serial-rule-method/README.md) has
a bounded source-to-typed-effects producer and shared native call/state analyses.
Source compilation retains explicit state reads/writes and Method calls.
Guarded nested Rules now enter this typed route even without Method declarations
or state. Native sampled admission verifies closed metadata, ordered known-one
prefixes, real D/E and domain routes, private terminal uses, Collection limits,
and per-occurrence sampling plans. Source bodies/interfaces cannot supply the
final-only attributes. The common lowerer now materializes these records for
all transactions, including literal true/false, state-free and read-only cases.
Original-source authentication checks captures, candidate versions, guard
identities/locations and exact affected-definition inventories before canonical
final verification checks the retained SSA, wiring and dependency closure.
Both proofs succeed before atomic replacement; mappings locate candidate
objects but do not supply semantic authority. The canonical final verifier
owns its independent budget and runs once at this lowering seam.

C++ and managed RTL consume the same sampling plan, freeze physical pins and
ordered checks during Work/PREPARE, and commit pending history only after
whole-system success. False-fire, held and falling samples maintain legal
history; Discard and failure preserve committed history. Focused final-IR
execution has native workers 1/2 and four-state RTL evidence. The public dynamic
source regression additionally covers independent and cross-owner transactions,
candidate-at-entry guards, actual state/history rollback and native sticky
failure recovery. These module-root tests do not establish Method `@system` closure;
historical checkpoint `44b888c5` does not certify the current candidate.

Within one immutable check-plan invocation, `SampledAdmissionValidation` shares
complete definition-local boundaries and successful Rule prefix proofs across
catalog and wiring checks. Keys preserve exact definitions and full bindings;
each query still parses the actual Rule. Actual instance routes, physical domains,
private uses and each distinct root remain independently checked. Changed IR
requires a new context; reuse does not change verification limits or admission.

Source units can publish these typed operations and their declaration-only
interfaces. Consumer compilation reads the interfaces, without loading provider
Python or Method bodies. Native final link lowers calls through ordinary
static instances with straight-line effects, a shared actual clock/reset
domain and one outer writer per physical scalar owner. Ordinary results escaping an outer Rule still
require proven-true guards; this restriction does not exempt them from sampling.
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

Native module imports require an explicit `semantic_kind`. Hardware imports
carry `dependency_summary`; transaction imports carry `transaction_summary`.
The two summaries are mutually exclusive, and transaction imports cannot claim
primitive behavior. Untagged imports are rejected. Hardware dependency analysis
rejects transaction imports throughout the actual instance closure, including
result-free children; final package checking also rejects unused transaction
imports. Publication alone does not establish hardware execution.

Source compilation defers the hardware-only record simplifier for packages
retaining transaction semantics. The existing `--source-import-out` diagnostic
still denotes lowered hardware and rejects these packages; published source
bodies retain their actual typed IR. Link checks complete source-unit authority,
then applies the common transaction lowerer. It rechecks canonical primitive
imports with the admitted registry; native transformation separately validates
their hardware schema. Each admitted state becomes one DFFE with original
clock/reset/init, and candidate D values come from isolated pure Rules. All
staged hardware checks succeed before the package becomes final.

The explicit `pycircuit-opt --ac-analyze-transaction-state-ssa` pass preserves
its input IR. Each outer transaction starts from old Q; nested Method calls
share that transaction's candidate versions while keeping ordinary SSA values
as snapshots. The native analysis requires actual Method bodies, straight-line
effect paths and paired clock/reset wire identity. It tracks ordered guard and
value-validity prefixes; false or unknown guards remain explicit obligations
for sampled lowering and checks. Unsupported selectors,
conditional effects and unproved domains reject this analysis only. No plan is
published if any transaction fails. Branch joins, partial updates, public
dynamic-source acceptance and resource handling remain unfinished.
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

## Retired routes

CycleAwareSignal/JIT, structural builders, Agentic Circuit/QueueGraph, `acc.py`,
`acc`, and `pycc` are retired. They are not aliases or fallback implementations.
