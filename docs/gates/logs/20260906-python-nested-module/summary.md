# Nested Python module lowering gate summary

Decision 0186 lowers one ordinary Python module call inside another module to
the hierarchy-preserving specialization path. Python continues to express typed
values and function calls only.

## Evidence

- `examples/agentic-circuit/pipelines/inferred_nested_module_pipeline.py`
  - `wrapper` returns `increment(value)` and the system calls `wrapper` twice.
- `QueueFrontendTest.test_nested_module_call_preserves_parent_child_structure`
  - verifies inferred Wrapper -> Increment `ac.instance` hierarchy.
- `QueueCodegenTest.test_nested_python_module_calls_preserve_reuse_and_run`
  - canonical plan nests one Increment specialization under Wrapper;
  - generated C++ emits one class per definition;
  - inputs `5,10` produce `6,11`.
- Python frontend suite: 73/73 passed.

## Remaining scope

Python module lowering still needs multi-statement internal graphs, stateful
modules, arbitrary arity, static parameters, inferred fanout, and incremental
activation.
