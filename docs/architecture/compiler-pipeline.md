# Compiler pipeline

The active compiler processes source one unit at a time, links an explicit
closure, verifies a common final design, then selects a backend:

1. `pycircuit compile -c source.py` captures exactly one Python source and
   publishes its body, interface, dependency file, and unit receipt.
2. Parent sources import compiler-published interfaces through explicit `-I`
   unit directories. The compiler does not fall back to child Python bodies.
3. `pycircuit link <units...> --top ...` validates the complete unit closure,
   resolves instances, and publishes one verified final design. `--target linked`
   instead saves normalized transactional ACIR after common analysis.
   `link --from-linked linked.ac` revalidates it and resumes the same pipeline.
4. `pycircuit emit <design_top.ac> --target cpp|verilog` verifies the final
   input and emits the chosen target.

## Semantic stage ownership

The source route has these boundaries:

1. **Import:** capture syntax, declaration constraints and source locations without
   executing Python design functions.
2. **Local constraints:** solve payload and Method requirements that can be
   resolved within the source unit; publish unresolved provider requirements.
3. **Link binding:** authenticate providers and the explicit instance closure.
4. **Closed constraints:** ACIR specializes Rule registrations, binds resource
   identity, allocates default storage and closes resource, Method and payload
   constraints.
5. **Common analysis:** derive old-Q reads, paths, admission, effects and conflicts
   from the linked transactional IR.
6. **Lowering:** route transactions, verify correspondence and refine storage.
   Linked, Routed (saved anchor) and Final are distinct artifact boundaries.
7. **Emission:** consume the verified final design in either backend.

The [source-unit workflow](../development/source-unit-workflow.md#current-compiler-ownership)
maps these stages to implementation owners. Validation at independent entries
and after transformations remains necessary; verification of unchanged linked
IR is shared with the common analysis entry.

## Python to Linked ACIR

Independent compilation consumes one Python file and explicitly supplied
dependency interfaces. Linking consumes the complete body/interface closure and
binds the actual instances and resources. A successful compile therefore does
not establish that the complete design can link or execute.

```text
Python file + dependency interfaces
  → stable source snapshot and AST capture
  → private ac.python_capture transport
  → native importer and local constraint solving
  → Unlinked body + authenticated interface
  → source-unit publication

Explicit source-unit closure + selected top
  → body/interface and provider authentication
  → declaration merge and System entry
  → instance specialization and reference binding
  → resource identity resolution and concrete allocation
  → closed Method and payload constraints
  → common transaction analysis and verification
  → published Linked ACIR
```

### Capture and native import

The Python driver reads a stable source snapshot, parses it with `ast.parse` and
encodes its AST, relative path and source spans in `ac.python_capture`. This is
temporary syntax transport, not the design's transactional IR. Capture does not
execute design functions or load provider Python to construct hardware.

The native importer reads that transport and the supplied interfaces. It builds
Module definitions, logical State/Queue resources, SSA computations, Rule
definitions and registrations, Methods and guards, instance references and
resource connection constraints. Ordinary Rules require explicit registration;
an autorule becomes an ordinary Rule with one registration. Supported
Module-level branches retain their registration selection as `scf.if`.

### Independent compilation and interface publication

`compilePythonSourceUnit` imports the source, solves locally decidable payload
and Method constraints, infers Module-reference requirements and verifies the
IR. Requirements needing actual providers or resource connections remain for
linking. Interface extraction replaces Module bodies with import declarations
and retains the types, resource signatures and Method interfaces consumers need.
The compiler verifies body/interface agreement before publication.

Each published unit contains `name.ac`, `name.interface.ac`, a dependency file
`name.d` and the owner/file receipt `unit.json`. Consumer compilation reads the
dependency interfaces, not provider implementation bodies. Link subsequently
requires both the bodies and their owning interfaces.

### Authentication and concrete binding

Link first checks artifact versions, stages, source owners and body/interface
agreement. A shared provider registry checks the explicit dependency closure
and imported snapshots before declaration merging. Actual definitions replace
matching Module imports; equivalent declarations are reconciled and conflicting
definitions reject. The selected top must currently be a closed concrete
Module. Link establishes its System entry and reset policy.

Reference binding traverses actual static instance occurrences and specializes
their Module definitions. It binds Module-reference formals and forwarded
references to existing instances without introducing another state owner.
Specialization retains Module hierarchy and Method bodies; it does not flatten
them into physical wiring.

Resource resolution specializes each Rule registration and propagates resource
identity through arguments, results, captures and aliases. Existing State/Queue
bindings retain their identity; unbound connections needing default storage
receive concrete Queues. Resource accesses become appropriate State/Queue
operations. Method and payload constraints are then solved against the actual
bindings, with closure required.

### Linked publication boundary

Before `--target linked` publishes, the shared continuation checks the source
owner inventory and selected System, then builds common transaction analysis.
That analysis checks actual instances, resources, calls, availability, admission,
conflicts and control conditions. A structural merge alone is not a publishable
Linked artifact.

The file retains nominal types, concrete instance/resource bindings, Module,
Rule, Method, guard, call and branch structure, System policy and source
provenance. Temporary analysis plans, indexes and borrowed IR pointers are not
serialized. Reload verifies the saved IR and rebuilds analysis without reopening
Python files or source-unit directories. Transaction routing and physical storage
lowering occur after this boundary.

```bash
pycircuit compile -c src/child.py --source-root src --package-prefix demo \
  -o units/child
pycircuit compile -c src/top.py --source-root src --package-prefix demo \
  -I units/child -o units/top
pycircuit link units/child units/top --top demo.top.Top \
  --target linked -o linked.ac
```

### Implementation owners

Paths below are relative to the repository root.

| Responsibility | Implementation |
| --- | --- |
| Stable capture and AST transport | `python/pycircuit/_source_capture.py`, `_source_transport.py` |
| Compile orchestration and source-unit publication | `python/pycircuit/_source_compile.py`; `compiler/tools/source-unit.cpp` |
| Native source import | `compiler/lib/Compiler/PythonImportRecords.cpp`, `PythonImportExpressions.inc`, `PythonImportStatements.inc` |
| Compile pipeline and interface extraction | `compiler/lib/Compiler/SourceUnit.cpp`; `compiler/lib/Transforms/ExtractSourceInterface.cpp` |
| Interface registry and body authentication | `compiler/lib/Compiler/SourceHeaderRegistry.cpp`, `SourceBodySnapshots.cpp` |
| Link orchestration, merge, instance binding and shared continuation | `python/pycircuit/_driver.py`; `compiler/tools/link.cpp`; `compiler/lib/Compiler/SourceLink.cpp` |
| Resource, Method and payload constraints | `compiler/lib/Dialect/ACIR/TransactionalResourceConstraints.cpp`, `TransactionalMethodConstraints.cpp`, `TransactionalPayloadConstraints.cpp` |
| Transaction IR contracts and common analysis | `compiler/lib/Dialect/ACIR/TransactionalContracts.cpp`, `TransactionalAnalysis.cpp` |
| Publication locks, owner validation and failed-output protection | `python/pycircuit/_publication.py`, `_native_verify.py` |

`SourceUnit.cpp` invokes the constraint solvers with incomplete provider closure
permitted; `SourceLink.cpp` invokes them again after concrete binding with closure
required. This is the same constraint machinery at two information boundaries.
The current Linked artifact envelope checks live in `continueLinkedSource`,
while `TransactionalAnalysis::create` owns transaction semantic analysis. A new
consumer must preserve both sets of checks.

## Downstream compilation

The same saved final design is input to both backends. No whole-system capture
followed by source splitting is allowed. C++ output keeps one implementation
source group per source unit, plus generated core/runtime glue. Generated CMake
compiles those units separately as `pycircuit_modules`; the host driver links
them with the shared Runtime.

Runtime-only CMake exports `pycircuit::pyc6_runtime` without LLVM discovery.
CompilerDev includes native compiler development targets and requires exact
LLVM/MLIR 22.1.8.

The common source and final hardware representation is ACIR. Its `ac` dialect
name is an IR identifier, not another Python frontend. Unsupported source or
backend capabilities fail before output publication; see
[known limitations](../development/known-limitations.md).
