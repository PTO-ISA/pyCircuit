# Framework boundary cleanup summary

## Scope

Decision 0235 restores a consumer-neutral pyCircuit boundary. The cleanup
removes the in-tree consumer design program, product decoder and scheduling
specialization, consumer payload and workload-trace schemas, reference model,
trace-driven run/workspace path, process trace operations, and every trace
method from the generated-model ABI.

Generic Cycle-Aware Signal, PYC, ACPy/ACIR/ACSim, Queue/Table/Reg semantics,
wide immutable values, gfsim, probes/observations, model plan/emit, and SDK
packaging remain.

## Results

- Full ACIR CMake build: PASS.
- Full ACIR CTest: PASS, 18/18 tests.
- pyCircuit unit tests: PASS, 91/91 tests.
- Agentic Circuit CLI and contract tests: PASS, 94 passed and 3 skipped.
- Fresh Python 3.11 SDK plan/emit/schema/external-consumer lane: PASS, 4 passed
  and 1 non-3.11-profile test skipped.
- Repository contract/schema/catalog check: PASS, 15 public schemas and 35
  standard-library components.
- API hygiene: PASS.
- Decision coverage and existing evidence validation: PASS, 235 rows with the
  two pre-existing deferred release decisions.
- Strict MkDocs build: PASS.
- Diff whitespace validation: PASS.
- Boundary scan: active framework source, public APIs, schemas, examples, and
  gates contain no consumer design, product payload/workload trace, or trace ABI
  surface. Historical decisions and gate logs retain provenance names only.

## ABI hard break

- `AgenticModelApiV1`: 96 bytes to 80 bytes.
- `AgenticModelStepResultV1`: 32 bytes to 24 bytes.
- Lifecycle: `create -> configure -> reset -> step`.
- Removed: trace loading, trace position/cursor, and observation/trace export.
