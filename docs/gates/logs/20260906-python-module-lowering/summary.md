# Python module lowering gate summary

Decision 0185 connects ordinary typed Python module definitions and calls to the
hierarchy-preserving QueueGraph/gfsim path. Python contains no instance,
specialization, Queue port, source/sink, readiness, or backpressure syntax.

## Evidence

- `examples/agentic-circuit/pipelines/inferred_module_pipeline.py`
  - defines one typed `increment` module and calls it twice normally.
- `QueueFrontendTest.test_typed_module_calls_lower_to_structured_reusable_acir`
  - verifies inferred `ac.system`, `ac.module`, and `ac.instance` structure.
- `QueueCodegenTest.test_typed_module_calls_generate_one_reusable_class_and_run`
  - runs the native MLIR plan/codegen path;
  - canonical plan contains one specialization and two instances;
  - inputs `5,10` produce `6,11`.
- Python frontend suite: 72/72 passed.

## Remaining scope

Python module lowering still needs stateful/nested modules, arbitrary arity,
static parameters, compiler-inferred repeated-value fanout, and incremental
activation.
