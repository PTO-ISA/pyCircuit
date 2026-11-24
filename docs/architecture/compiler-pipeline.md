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

## Handwritten structured GFIR in CompilerDev

The CompilerDev tools accept a closed, handwritten structured package
with `gfir.contract = 1 : i64` and one `gfir.root` of kind `module`.
`gfir.module` defines the typed graph, `gfir.instance` names the child, and
`gfir.call` binds its input and result values. The structured verifier derives
field dependencies from actual SSA, including links between child instances.

```sh
pycircuit-opt graph.gfir.mlir --gfir-verify -o checked.gfir.mlir
pycircuit-emit checked.gfir.mlir --input-stage gfir --target cpp > behavior.json
pycircuit-opt checked.gfir.mlir --gfir-to-hwir --ac-verify-hardware -o final.mlir
pycircuit-emit final.mlir --target cpp > final-cpp.json
pycircuit-emit final.mlir --target verilog > rtl.json
```

The profile supports Bits and Struct combinational graphs. A clocked Module
root may declare one generated rising `default` domain. Clocked children
inherit that domain through their instances. The verifier schedules early
outputs from committed state and pure graph dependencies separately from
outputs that require a child's Rule evaluation. A clocked child prepares
once per Work and remains pending until the root commits the sample.
Clocked modules support
native FIFO v1, logical variables of Bits, Struct, or rank-one Table type,
straight-line Rules, and local Methods called with
`gfir.call self["interface"]`. A Rule explicitly captures a graph value with
`%arg = %actual : T`. The verifier schedules captured graph fields before
the Rule and exposes snapshots of its declared results as
`gfir.result @rule[0] : T` when its resource admission is unconditional.
An indexed write spells the index type inside
the brackets, as in `gfir.write @entries[%i : !Index], %entry : !Entry`.
The optional `execution_policy` on `gfir.root` selects `strict` or `if_one`
reset interpretation and projects to `ac.execution_policy` on the HWIR root.

Both native branches verify the structured input. The hardware branch also
verifies the generated HWIR and requests managed sampling for a clocked root.
Behavior C++ derives admission from committed resource state and gives each
Rule its own candidate frame. When the root owns logical variables, it samples
their shared clock history once. A Rule whose result is used runs in Work so
its snapshot is visible at each sample. Rules that only prepare effects can
wait for a non-reset rising edge. Held and falling samples still publish
outputs and advance committed clock history. FIFO drives go through the same
gfsim kernel on every sample.

`SimSystem` commits or discards the complete sample. For the generated root
with no observations or additional checks, it can call the concrete root
directly. Additional roots or active observation slots use the whole-system
check before any commit. The hand-written
`TableRule` package uses the original C++ driver against both generated
models. It covers backpressure, four-state Table indices, rollback, and reset.
The clocked-feedback fixture checks separate instance state, bidirectional
old-state feedback and a late resource failure before root commit.

RAM, System roots, cross-instance Methods, and resource-gated Rule results are
subsequent work. Python source compilation uses the routed GFIR and saved-anchor
contract. See [known limitations](../development/known-limitations.md).
