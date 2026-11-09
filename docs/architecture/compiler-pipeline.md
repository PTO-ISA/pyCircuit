# Compiler pipeline

The active compiler processes source one unit at a time, links an explicit
closure, verifies a common final design, then selects a backend:

1. `pycircuit compile -c source.py` captures exactly one Python source and
   publishes its body, interface, dependency file, and unit receipt.
2. Parent sources import compiler-published interfaces through explicit `-I`
   unit directories. The compiler does not fall back to child Python bodies.
3. `pycircuit link <units...> --top ...` validates the complete unit closure,
   resolves instances, and publishes one verified final design.
4. `pycircuit emit <design_top.ac> --target structural-cpp|verilog` verifies
   the saved final ACIR, lowers it once through `ac-final-to-hwir`, verifies
   independent HWIR v2, and emits the chosen target from a concrete design
   plan. Native `pycircuit-emit` accepts HWIR rather than final ACIR.

The same saved final design and the same verified HWIR lowering are input to
both backends. No whole-system capture followed by source splitting is allowed.
C++ output keeps one implementation source group per source unit, plus generated
core/runtime glue. A source group may additionally own bounded
`work_unit_<N>_<M>.cpp` files when a module schedule is too large for header-only
Work helpers. Generated CMake compiles every source-unit implementation and Work
shard separately as `pycircuit_modules`; the host driver links them with the
shared Runtime.

Runtime-only CMake exports `pycircuit::pyc6_runtime` without LLVM discovery.
CompilerDev includes native compiler development targets and requires exact
LLVM/MLIR 22.1.8.

ACIR remains the linked public artifact; independent HWIR v2 is the closed
backend contract. Its dialect names are IR identifiers, not another Python
frontend. Unsupported source or backend capabilities fail before output
publication; see
[known limitations](../development/known-limitations.md).
