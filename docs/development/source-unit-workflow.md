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
| Simplify and describe | `SimplifyRecordWires`, `ExtractSourceInterface` | Simplify immutable record wiring and derive the public interface from verified hardware. Preserve nominal identity, dependencies and source ownership. |
| Verify published authority | `SourceHeaderRegistry`, `SourceBodySnapshots` | Check provider declarations, imports and body/interface agreement. An imported declaration does not acquire a caller's defaults or type authority. |
| Link | `_driver.py`, `SourceLink.cpp` | Consume the complete explicit unit closure, resolve imports to definitions, construct the selected root and verify the final hardware package. |
| Common analysis | `HardwareAnalysis` and its dependency, source-check and Work-partition owners | Share packed widths, nominal field layouts, dependency/effect checks and scheduling facts between targets. |
| Emit | `_emit.py`, `pycircuit-emit.cpp`, existing target definition emitters | Consume the same verified final snapshot and realize C++ field planes or packed Verilog values. Retain source-owned output inventories and generated CMake metadata. |
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
