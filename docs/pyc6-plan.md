# pyCircuit 6 Evolution Plan

This is the active pyCircuit 6 implementation plan. Historical implementation
details remain in Git history and archived gate evidence; they are not current
product commitments.

## Product boundary

pyCircuit is a programming, compiler, runtime, and backend framework. It owns:

- the `pycircuit` and `agentic_circuit` authoring surfaces;
- semantic types and primitives;
- PYC, ACIR, and ACSim dialects, verifiers, and passes;
- generic C++/Verilog/gfsim backends;
- vendor-neutral examples and regression fixtures; and
- compiler/runtime packaging and release gates.

Consumer designs, ISA/opcode catalogs, architectural payloads, ELF loaders,
reference models, model-comparison tools, serialized workload traces, trace
adapters, and product-specific testbenches are out of tree. There is no design
exception. The generated-model ABI has no trace loading, trace cursor,
trace-position, or trace/observation export surface (Decision 0235).

## Current priorities

### 1. Preserve the pyCircuit 6 language contract

- Keep `CycleAwareSignal`, `CycleAwareDomain`, timed-domain authoring, and
  automatic cycle balancing as first-class contracts.
- Keep structural and cycle-aware authoring on one verified PYC semantics path.
- Enforce semantic changes in dialect verifiers or passes before backend code.
- Keep canonical PYC vendor-neutral; implementation selection is backend-owned.
- Preserve C++/Verilog equivalence at the documented observation boundaries.

### 2. Complete generic Agentic Circuit semantics

- Keep typed payloads, recursive aggregates, exact-width bit operations,
  immutable updates, and nominal identity verified across ACPy and ACIR.
- Keep Queue/Table/Reg effects in inferred prepare/publish/no-fail commit groups.
- Complete the generic firing/transition contract tracked by issue #28.
- Complete generic Table lowering and parity work tracked by issues #22–#25.
- Express multi-lane ordered Queue behavior without processor-stage or
  design-specific terminology.

### 3. Publish a consumer-neutral SDK

- Provide installed `model plan` and `model emit-cpp` commands using the shared
  frontend, verifier, and QueueGraph path.
- Keep model plans, manifests, source hashes, generated-source lists, and
  depfiles deterministic and root-independent.
- Split Runtime and CompilerDev package dependencies so ordinary generated
  models do not require LLVM/MLIR development packages.
- Keep the public runtime lifecycle limited to create, configure, reset, step,
  statistics, and error reporting.
- Validate the SDK with an external vendor-neutral fixture containing no ISA,
  product, trace, or reference-model contract.

### 4. Keep framework gates independent

- Required PR CI remains lightweight.
- Native or semantic PRs attach the narrowest focused evidence.
- Release closure runs the complete integrated AC/PYC matrix.
- No framework gate checks out, imports, builds, or executes a consumer design
  or reference model.
- Consumer-originated failures must first be reduced to vendor-neutral fixtures.

## Gate mapping

Use the minimum applicable lanes from
[`testing-and-gates.md`](development/testing-and-gates.md).

| Change | Required evidence |
| --- | --- |
| Documentation or governance | changed-file checks, API hygiene, strict docs build |
| Cycle-aware frontend or inference | unit tests, API hygiene, examples, semantic regressions |
| MLIR semantics or legality | focused lit/CTest plus strict decision status |
| C++ or Verilog behavior | focused backend execution and applicable equivalence evidence |
| ACIR or Agentic Circuit | frontend contracts, ACIR/ACSim tests, QueueGraph/gfsim, applicable PYC parity |
| Packaging or installed SDK | schema/contract checks and relocated external consumer smoke |

Use one `PYC_GATE_RUN_ID` for related semantic lanes. Record skipped gates and
their risk in the pull request.

## Completion criteria

The current cleanup and SDK milestone is complete when:

- active source, tests, examples, schemas, build files, and gates contain no
  consumer or product implementation;
- no public model/runtime ABI exposes trace input, cursor, position, or output;
- Decisions 0222, 0229, and 0230 are superseded and Decision 0235 is verified;
- the generic frontend, verifier, QueueGraph/gfsim, PYC, SDK, and documentation
  gates pass from the current checkout; and
- consumer compatibility is tested only in the consumer repository against a
  pinned pyCircuit revision.
