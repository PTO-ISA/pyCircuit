# Physical-edge RTL export proposal

Status: independently reviewed, ready for bounded implementation after the
review corrections below. This does not claim
current compiler support. It addresses C08 in the compiler benchmark audit.

## Interface and execution policy

Add explicit `pycircuit emit --target verilog --rtl-mode physical`. Omitting the
option retains existing managed execution and generated Systems. Reject physical
mode for C++ and closed Systems. Both exports consume the same verified final IR;
retain its source checks and authenticated sampled inventories unchanged.

Physical mode exports the existing data/results and compiler-owned clock/reset
ports. No frontend or external phase/commit controls, gated clocks or delays.
Each valid rising edge computes one update from old Q using existing D/E/reset
semantics. All root state endpoints observe one settled pre-edge permission;
an error in any subtree prevents every state update. State changes use NBA.

This policy is physical-edge execution, not a host Work/PREPARE/Discard interface.
Managed mode retains arbitrary host sampling, discard/retry and Reset semantics.
Physical simulation faults are sticky until simulation reconstruction; asserting
physical reset does not recover a failed simulation. Physical export initializes
state from each declared initializer after common analysis proves the existing
closed literal/constructor DAG under every actual binding. Typed elaboration
constants feed a physical-only primitive parameter; startup does not sample a
live initializer wire, add delays or inject reset pulses. Ordinary managed and
autonomous primitive defaults retain their existing initialization policy.
Dynamic primitive initializers require managed execution.

## Common analysis and existing owners

A read-only physical projection of `HardwareSourceCheckPlan` must establish:

- ordinary root with one actual clock/reset pair, proved through real instance
  and Collection bindings, not port names;
- all state endpoints are existing DFF/DFFE leaves, including Collections;
- no data/combinational use of clock and no derived clocks;
- residual checks are guard-known checks; other checks must be proved nonfailing
  by common SSA/binding analysis or rejected as requiring managed execution;
- no ordinary-module observations, unsupported primitive or unresolved domain.

Enumerate both sampling bindings and physical storage endpoints, including
stateless/readonly transactions. Each binding must resolve to the same actual
root pair. Root prospective error includes unknown(root_reset) independently
of the presence of storage; sample this error only on an active edge.

Do not delete `ac.expect` or sampled metadata. Render retained guard-known
predicates combinationally and aggregate with existing leaf-control errors.
The knownness error is unknown(reached), or, when reset is known zero and reached
is known one, unknown(predicate). A known false guard retains hold semantics.
Constant true guards fold naturally; proving knownness never proves true fire.
Guard-known controls include both callable guards and branch selectors from the
current authenticated controls schema. Preserve actual raw/reached operands and
ordered paths; do not infer them from generated enables. Any other check may be
discharged only with a common proof covering every applicable input and X/Z case.

Reuse `dff.v::next_q` and `dffe.v::next_q/edge_control_error` in a checked-autonomous
selection distinct from managed phases. Each leaf exposes prospective rising-edge
control error and commits only if `clk === 1'b1` and internal root permission is
exactly one. Internal permission is independent of clock level and combines all
errors before any NBA storage update.
Prospective error depends on reset/enable and retained check operands, never on
permission, sticky state or clock level. Functional fire and D/E must also remain
independent of diagnostic permission, avoiding combinational feedback.

One root verification-only sticky flag records source/primitive faults on rising
edges and clock transitions to X/Z. Every leaf also checks known-one clock, so
Verilog's 0-to-X/Z posedge cannot commit before sticky state takes effect. Report
failure once; permit inspection after NBA to prove zero commit. Inputs and controls
must settle before the edge; contradictory clock transitions in one time slot
are outside the existing leaf timing contract.

Report the exact compiler-owned diagnostic `PYCIRCUIT_PHYSICAL_FAULT` once when
sticky failure is set. Simulations may continue for post-NBA state inspection;
verification/benchmark runners must reject this diagnostic even if the unchanged
upstream TB later prints success or exits zero. No authored status port is added.

Time-zero testbench setup establishes a known clock and settled controls before
the first exercised edge. Initial X-to-known setup is initialization, not an
invalid-clock transition. Consistent with existing autonomous leaves, initial
X-to-one can cause a posedge update if reset/controls are settled and valid; tests
that need a clean first low/high cycle must initialize clock low. After setup,
any transition into X/Z sets sticky failure, and X/Z-to-one cannot recover it.

Synthesis retains functional fire, D/E/reset and payload logic. X/Z diagnostics
and sticky failure history are verification-only, as in existing leaves. Never
run acceptance simulations with SYNTHESIS defined to suppress failures.

## Implementation decomposition

1. Common analysis projection in HardwareAnalysis/HardwareSourceChecks, using
   existing sampled identities, actual bindings and source-check inventory.
2. RTL context distinguishes managed lifecycle from physical error aggregation;
   ordinary definition/wrapper emitters consume the verified projection.
3. DFF/DFFE checked-autonomous branch reuses transition/control-error functions.
4. Native/Python emit option and receipt record actual export policy; metadata
   reports exact public ports/domain roles. Keep internal identifier legalization.
5. Independent tests, active docs and benchmark runner select physical mode for
   its edge-driven oracle. No benchmark-specific compiler decisions.

FIFO/memory fault aggregation is additional existing-primitive work, not silently
admitted by this first projection. Track that limit explicitly.

## Required acceptance

- Original dff8 TB, counters, guarded updates, independent writers, nested
  instances and register Collections, without added cycles.
- Successful known-clock traces agree with native workers1/2 and managed RTL at
  corresponding low/high samples and equivalent initialized states.
- Real four-state payload, reset, enable, ordered guard and selector tests.
- A failing later subtree prevents an earlier subtree's valid proposed write;
  observe state after NBA and on subsequent edges.
- 0-to-X, 0-to-Z and high-to-unknown clock transitions never commit; subsequent
  known clocks/reset cannot recover a failed execution.
- Held/falling unknown guard/reset retain active-edge masking.
- Unsupported checks/primitives, multiple domains and clock-data use reject with
  unchanged published outputs.
- Existing generated-System and managed discard/history tests still pass.
- Simulation without SYNTHESIS plus separate synthesis elaboration showing no
  extra functional storage.
- Stateless reset-X/Z, initial clock setup, conditional selector ordering, and
  diagnostic-plus-upstream-success rejection are independent regression cases.

Architecture proposal: independent architect gpt-6-astra. Independent planner
gpt-6-astra reviewed and requested root reset validation without leaves and a
runner-visible fault marker; both corrections and startup/order clarifications
are incorporated here. Exact candidate execution and implementation review
remain required.
