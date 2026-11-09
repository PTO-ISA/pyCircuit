# Independent HWIR to Verilog contract

The production Verilog backend consumes the same independent HWIR package as
Structural C++.  Final ACIR is a lowering input only; it is not a Verilog
backend API and the renderer must never reconstruct ACIR analyses.

```text
Python -> linked final ACIR -> ac-final-to-hwir -> verified HWIR
                                                -> VerilogDesignPlan
                                                -> managed/physical Verilog
```

## Boundary and ownership

`hwir.contract = 2 : i64` is the first contract that carries the complete RTL
boundary.  Version 1 remains readable only for focused migration fixtures.
The canonical final-ACIR conversion emits version 2.

* `hwir.module` owns functional ports, graph operations and source ownership.
* `hwir.interface` records the public physical prefix and root-only defaults.
  Internal transaction-lowering ports are never public RTL ports.
* `hwir.domain_inputs` records definition-local clock/reset input ordinals.
* `hwir.root_kind = "system"` marks a closed generated System.  Absence means
  an ordinary Module, which is the only root admitted by physical RTL export.
* `hwir.rule_domain` records the clock/reset capture ordinals of every sampled
  rule, including stateless and read-only transactions.
* `hwir.expect` retains raw condition, reached path and, for `guard_known`, the
  sampled clock/reset values.  ACIR sampled-admission and transaction-wiring
  proof records do not cross the boundary.
* `hwir.system.entry` selects the implementation root.  Optional
  `hwir.logical_entry` preserves the authored entry when a stateless entry
  adapter is selected.

The ACIR-to-HWIR conversion is responsible for authenticating and translating
these facts.  HWIR verification rechecks their closed shape after serialization.

## Planning and rendering

`VerilogDesignPlan` is immutable after construction and contains concrete
widths, names, ports, occurrence bindings, primitive parameters, retained
effects, source ownership and wrapper policy.  The renderer may select nets by
plan ID and format text; it may not query `HardwareAnalysis`, `ac::ModuleOp`,
`ac.sampled_admission` or `ac.transaction_wiring`.

Managed and physical are export policies over one functional HWIR graph:

| Policy | External lifecycle | State update | Diagnostics |
| --- | --- | --- | --- |
| managed | phase/prepare/commit/discard | host-authorized whole-system commit | existing managed check lifecycle |
| physical | public data plus compiler clock/reset only | one atomic update per valid rising edge | sticky simulation-only `PYCIRCUIT_PHYSICAL_FAULT` |

Physical admission accepts DFF/DFFE, FIFO, byte-addressed memory, synchronous
1R1W memory and synchronous 2R1W memory, plus `guard_known` checks. Every
primitive contributes a combinational edge-error signal to one root commit
permission, so an unknown reset/control/enabled address/write payload/strobe
causes a whole-design zero commit and a sticky simulation fault.

Exact `ac.bits.constant4` values remain three independent value/known/Z planes
in the concrete plan. Managed and physical Verilog render the corresponding
`W'b...xz...` literal without collapsing unknown symbols. A static DFF/DFFE
initializer uses the same three-plane evaluator, including widths above 64;
zero extension marks the added high bits as known zero. Dynamic initializers
remain outside physical admission.

Physical observations are emitted as a registered sideband channel. Each
stable observation ordinal owns one `valid` and `path` bit and one fixed-width
slot in the value/known/Z planes. Definition-local observation nets are routed
through explicit module ports to the root; the wrapper does not depend on
hierarchical references. Static identities, grouping, kind and payload widths
are described by `ports.json` under `observation_channel` and `observations`.
The channel publishes only on an admitted rising edge, clears during reset or
a rejected edge, preserves four-state event/log payloads, and requires a known
active gauge/report value.

Closed Systems, derived/multiple clocks, dynamic DFF initializers, unsupported
primitives and residual checks beyond `guard_known` still fail closed.

## Migration gates

1. Freeze this contract and managed output baselines.
2. Emit and independently verify HWIR v2 facts.
3. Materialize a concrete Verilog plan.
4. Reach managed behavioral and publication parity from HWIR.
5. Add independent HWIR physical admission.
6. Add physical rendering and checked DFF/DFFE primitives.
7. Switch the Python/CLI/publication route to HWIR with no silent fallback.
8. Remove the final-ACIR Verilog production entry and run gate/nightly
   acceptance as applicable.

Each gate preserves failed-output protection.  A failure after the public
switch is fixed or the switch commit is rolled back; it never dispatches to the
retired ACIR emitter.
