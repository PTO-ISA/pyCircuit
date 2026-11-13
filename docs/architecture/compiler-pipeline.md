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

## IR stages and library boundaries

The compiler separates its existing source, routed and final stages into
ACIR → GFIR (General Flow IR) → HWIR:

| Layer | Artifact stage | Information and consumers |
| --- | --- | --- |
| ACIR | Source unit | Declarations, typed hierarchy, state accesses, Methods and transactions for source analysis and routing. |
| GFIR | Routed anchor | Resolved hierarchy and SSA, normalized Rules, logical state/candidate updates, and checked routing, availability and effect records. Saved anchors can be reloaded and verified independently. |
| HWIR | Final hardware | Physical storage instances and verified pins after logical-state refinement; the common input to the current C++ and Verilog backends. |

Layer names do not require distinct spellings for every operation. Shared
structure, values and types retain `ac.*` / `!ac.*`. Source `ac.state`
declarations become `gfir.state` / `gfir.state.update` in GFIR. HWIR names the
final stage using the existing hardware operations; it does not introduce a
complete `hwir.*` dialect.

`ACIRCore` owns shared types, attributes and grammar contracts. `ACIRDialect`
owns source/shared operations and local verification; `GFIRDialect` owns the
routed logical-state operations and depends on the core and
`PycircuitStateInterfaces`. Shared binding analysis, budgets and proofs remain
in `ACIRAnalysis`, which consumes logical states through those interfaces.

`GFIRValidation` owns routed package verification (`verifyRoutedPackage`,
`pycircuit-opt --ac-verify-routed`); `HWIRValidation` owns final verification
(`verifyHardwarePackage`, `pycircuit-opt --ac-verify-hardware`). Both reuse
`ACIRAnalysis` and have no dependency on each other. Configure-time dependency
checks and standalone consumers enforce these library boundaries.

`ACIRToGFIR` owns `ac-route-transactions` and source-to-routed authentication;
`GFIRToHWIR` owns `ac-lower-transactions` and logical-to-physical correspondence.
Their implementations live in `lib/Conversion/ACIRToGFIR/` and
`lib/Conversion/GFIRToHWIR/`, sharing facilities through `ACIRConversionCommon`.
The conversion targets have no dependency on each other or on the emitters;
tools assemble the required pass registrations explicitly.

GFIR-to-high-level-C++ and GFIR-to-DOT consumers remain future work. The current
C++ and Verilog backends continue to consume the same verified HWIR artifact.
Unsupported source or backend capabilities fail before output publication; see
[known limitations](../development/known-limitations.md).
