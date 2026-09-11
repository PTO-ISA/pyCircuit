# Python stateful module gate summary

Decision 0187 lowers a normal typed lexical variable inside `@ac.module`
through `ac.var`, MLIR storage selection and rule analysis, then reuses one
generated specialization class with independent persistent state per instance.

## Naming contract

- Product Python, IR, tests, and current documentation use `ac.var` as the sole
  variable family and expose no long-form alias.
- Passes consume the public `ACDataFlowAnalyzer` API. MLIR's generic
  `DataFlowSolver` is private implementation detail only.

## Evidence

- `ACDataFlowAnalyzerTest.*`: 2/2 passed.
- Python Queue frontend suite: 74/74 passed.
- `variable-module-state-lowering.mlir`: passed lexical variable resolution,
  storage selection, rule lowering, freeze, planning, generated C++ checks,
  and C++ syntax compilation.
- Full ACIR lit suite: 159/159 passed.
- Native ModelAnalysis and CodeGen suites: 19/19 and 105/105 passed.
- `QueueCodegenTest.test_stateful_python_module_reuses_class_with_independent_state`:
  passed native frontend-to-gfsim generation and execution.
- Full Queue codegen integration suite: 18 passed, 1 skipped because the
  optional DavinciOO reference trace fixture was unavailable; pure, nested,
  and stateful Python module integration passed 3/3.
- Repository contracts, strict decision status, MkDocs strict, Python format,
  changed-file lint, C++ format, and `git diff --check`: passed.
- Runtime observations: the left instance consumes `1,2` and reports `1,3`;
  the right instance independently consumes `10` and reports `10`.

## Remaining scope

Decisions 0188 and 0189 subsequently add multiple lexical state variables and
direct input/output arity. Conditional updates, arbitrary internal graphs,
static parameters, compiler-inferred fanout, and incremental activation remain.
