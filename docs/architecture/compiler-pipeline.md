# Compiler pipeline

The active compiler processes source one unit at a time, links an explicit
closure, verifies a common final design, then selects a backend:

1. `pycircuit compile -c source.py` captures exactly one Python source and
   publishes its body, interface, dependency file, and unit receipt.
2. Parent sources import compiler-published interfaces through explicit `-I`
   unit directories. The compiler does not fall back to child Python bodies.
3. `pycircuit link <units...> --top ...` validates the complete unit closure,
   resolves instances, and publishes one verified final design.
4. `pycircuit emit <design_top.ac> --target cpp|verilog` verifies the final
   input and emits the chosen target.

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
