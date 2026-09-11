# Changelog

This project is still in an early prototype stage; APIs and generated outputs may change frequently.

## 6.0.0 (in development)

- Establish Cycle-Aware Signal as the primary pyCircuit 6 authoring model.
- Unify structural and cycle-aware authoring on verified PYC semantics and the
  `libpyc6_runtime` C++/Verilog backend contract.
- Integrate Agentic Circuit as the separate ACPy/ACIR frontend and gfsim model
  runtime within the canonical PTO-ISA/pyCircuit repository.

## Unreleased

- Add `pyc.concat` lowering for readable `{a, b, c}` packed concatenations in generated Verilog and C++.
- Improve generated identifier readability and traceability (scope + file/line name mangling).
- C++ emitter: add default-on hierarchical instance input-change cache to skip redundant submodule `eval()` calls; add `PYC_DISABLE_INSTANCE_EVAL_CACHE` override for A/B checks.
- C++ emitter: preserve instance eval-cache across `tick_commit()` for combinational-only callee hierarchies (stateful callees still invalidate for correctness).
- C++ emitter/runtime: add simulator perf controls `PYC_SIM_STATS`, `PYC_SIM_STATS_PATH`, and `PYC_SIM_FAST` (fidelity default).
- C++ emitter: add optional SCC worklist fallback scheduling path for cyclic graphs (`-DPYC_DISABLE_SCC_WORKLIST_EVAL` to force legacy loop).
- C++ emitter: add versioned input cache path (`-DPYC_DISABLE_VERSIONED_INPUT_CACHE` to force pure value-compare checks).
- Runtime primitives: add low-overhead unchanged-input/output fast paths in `pyc_byte_mem`, `pyc_fifo`, and `pyc_async_fifo`.
