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
- Implement Decision 0236 first: `@ac.rule` remains the only public scheduling
  boundary while the compiler forms complete Queue/Table/Reg/Slot
  prepare/publish/no-fail commit groups for issue #28.
- Implement Decision 0237 writer proof and deterministic arbitration for issue
  #25, then Decision 0238 multidimensional shape, typed initialization, and
  masked domains for issue #23.
- Implement Decision 0239 static-tuple multi-selection for issue #24. Preserve
  scalar `TableChoice` for omitted `count` and `count=1`; make every formed
  multi-lane valid prefix one atomic transaction.
- Implement Decision 0240 ordered multi-lane Queue identity and whole-prefix
  transfer for issue #21 without processor-stage or design-specific terminology.
- Complete Decision 0241 last in the semantic train: admit only the bounded
  Table profile through an explicit canonical-PYC register bank and prove C++ /
  Verilog parity for issue #22.

Decisions 0236–0241 are Accepted but remain `deferred` in the decision-status
table until their implementations and concrete gate evidence land. The frozen
order is #28 -> #25/#23 -> #24/#21 -> #22; #21 may proceed after #28 while the
Table slices converge.

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
- Implement Decision 0232 with exactly four release wheel assets: one
  platform-specific `pycircuit-hisi` wheel for each supported platform and one
  universal wheel each for `pycircuit-semantic-core` and `agentic-circuit`.
- Implement Decision 0234 Part A as one source-SHA-pinned manual workflow whose
  single candidate-acceptance barrier gates annotated-tag creation and every
  publish job. Publish jobs consume retained accepted bytes and never rebuild.
- Treat Decision 0234 Part B as the release-instance stop condition: redownload
  stable GitHub Release URLs on both platforms, rerun the relocated consumer,
  and record an immutable Actions/check-run attestation outside the release
  asset set.

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

The pyCircuit 6.0.0 milestone is complete when:

- active source, tests, examples, schemas, build files, and gates contain no
  consumer or product implementation;
- no public model/runtime ABI exposes trace input, cursor, position, or output;
- Decisions 0222, 0229, and 0230 are superseded and Decision 0235 is verified;
- issues #21, #22, #23, #24, #25, and #28 have implemented-verified evidence for
  Decisions 0236–0241 and are closed;
- Decisions 0232 and 0234 Part A have implemented-verified repository evidence,
  the exact four-wheel candidate map, and a passing release-workflow DAG;
- the generic frontend, verifier, QueueGraph/gfsim, PYC C++, Verilog, SDK, and
  documentation gates pass from the final upstream source SHA;
- annotated tag `v6.0.0` peels to that SHA and the non-draft,
  non-prerelease release publishes only accepted bytes;
- stable-URL post-download verification and the relocated generic consumer pass
  on Linux x86_64 and macOS arm64, with an immutable external attestation; and
- issue #61 closes with tag, source SHA, release index, hashes, supported
  platforms, and post-download evidence, while consumer compatibility remains
  in the consumer repository against a pinned pyCircuit revision.
