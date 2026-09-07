# Issue #40 P1 compile entry convergence

`compile_cycle_aware()` now always uses AST/JIT compilation and returns a
hardened `Design`. `build_cycle_aware()` is the explicit direct-Python
elaboration entry and always returns `CycleAwareCircuit`; its `emit_mlir()` is
backed by a hardened `Design` with canonical parameter metadata.

The old `eager` mode switch and public `structural`, `value_params`, and
`design_ctx` call options are rejected. Structural intent remains
decorator-owned. Direct Python elaboration rejects runtime value ports because
it cannot represent them honestly; those modules use the canonical JIT entry.

Parameterized hierarchical `domain.call()` sites now use a canonical parameter
digest in the child symbol. A two-specialization case passes strict `pycc` and
emits two distinct callees. Blank and whitespace names resolve identically in
both entrypoints.

All 34 migrated example/design `__main__` entrypoints execute successfully.
The previously failing `wire_ops` CycleAware ternary now uses `mux()` and passes
both generated C++ and Verilator; `jit_pipeline_vec` uses the same canonical
runtime-selection form.
