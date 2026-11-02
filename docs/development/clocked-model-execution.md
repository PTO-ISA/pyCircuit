# Complete-state clocked execution

One timing-bearing source can select `functional` or `timing` execution while
retaining the same computation and state owner. C++ and Verilog are independent
backend choices. This profile complements the bounded request/FIFO projection
in [shared execution models](shared-functional-timing-model.md).

## Source contract and admission

Use `@ac.module(execution={"profile": "clocked_fixedpoint_v1", "state_output": 1})`.
The root has one data input, one level/event output and a second output which
is the identity of its entire sole finite aggregate state owner. Bits, closed
Structs and admitted Tables retain their existing representation. The source
root owns one unconditional atomic Rule; pure owned child Modules/total Rules
and ordinary source-unit providers are allowed. Each actual clock/reset edge
must be the same domain. Clock/reset as data, references, parameterized or cyclic
hierarchy, hidden child state, resources and source observations are rejected.
The linker proves ownership and complete-witness identity from actual SSA;
metadata or model names cannot substitute for this proof.

`ExecutionProjection.cpp` validates the source slice before ordinary transaction
analysis, routing and storage inference. Publication/body-interface validation,
source checks and original-source materialization proof remain active. Neither
backend contains a model-name or instruction-address dispatch.

The author writes computation and state transitions once. The compiler infers
storage and lifecycle wiring. Independent result tuples and Struct interfaces
retain normal source-unit capture, explicit link closure and source provenance.

## Execution policy

Timing executes every full model tick. Functional may omit a run of repeated
idle ticks only after a real full tick establishes all of the following:

- The complete committed witness is fully known and unchanged.
- No declared event or memory handshake occurred.
- Inputs remain unchanged until the next explicit environment boundary.
- No failure or reset is hidden inside that gap.

The same generated DUT performs every necessary state transition. Latency is
not set to zero and no host arithmetic provides model results. A visible timer
changes the witness and prevents folding. A repeated event cannot be dropped
merely because other values repeat. Unknown control fails before commit;
inactive payload is governed by the ordinary four-state/availability contract.
Virtual model cycles, executed ticks, logical events and wall time are distinct.
This is deterministic stutter reduction, not arbitrary timing-IR projection or
a second handwritten functional model.

## Installed run adapter

After installing CompilerDev/Runtime, use the source-owned example project:

```sh
"$PYC_TOOLCHAIN_ROOT/bin/pycircuit" run examples/clocked_accumulator \
  --model clocked_accumulator.model.Accumulator --execution functional \
  --target cpp --memory examples/clocked_accumulator/simulation.json \
  --build-dir .pycircuit_out/acc-functional
```

The example README includes the four-word image creation command. Select
`timing` and/or `verilog` with separate build directories. Verilog uses physical
export and Icarus. No private driver or absolute workspace path is required.

The explicit `pycircuit-clocked-memory-v1` JSON schema supplies image/base/size,
finite instruction/data latency sequences, cycle/reset schedule, two independent
blocking-word-memory channel role maps and event valid/stop fields. The adapter
requires flat IO records. A valid request starts service and remains stable
until completion-ready; writes occur once at successful completion, respecting
byte enables and faults. Reset cancels outstanding platform requests before
external effects. The adapter interprets no instruction or numeric operation.

Native uses generated typed DUT/System lifecycle and whole-system prechecks.
Physical RTL uses normal clock/reset and packed ports. Verification-only faults
are distinct from synthesized architecture; ASIC state requires reset. Failed
new attempts invalidate old success claims with an incomplete receipt and
unknown counts. Generated publication inventory is validated before consumption.
Header-only source units do not create phantom translation-unit dependencies.

## Independent non-CPU regression

`tests/system/test_clocked_fixedpoint.py` copies the public accumulator project
into a clean directory and uses installed commands. The ordinary pure provider
maps3,5,7,11 to4,6,8,12; totals must be4,10,18,30 and the complete saved Table
must match. With alternating4/8-cycle waits, event cycles are4,13,18,27:
timing executes28 ticks, functional executes8 and skips20 idle ticks.

The same test preserves the source C++/RTL/state expectations, checks native and
physical-RTL inactive/active unknown boundaries, rejects partial witnesses and
hidden state, checks helper-only incremental reuse and stale-success protection,
and adds a visible timer which must execute all28 ticks with zero folding.
These are bounded profile checks, not full hardware/ISA certification.

## Boundaries

Multiple independent state owners, resource arbitration/cancellation, arbitrary
cross-domain scheduling and architectural time-dependent ordering are not
silently abstracted. Broader projection needs its own ownership/effects/order
proof. This tool extension has no CPU-source dependency. Consumer-specific
algorithms, firmware, ISS adapters and technology characterization remain in
their consumer/example scope.

Independent review remains NOT_RUN after the configured review service returned
HTTP401 before an opinion. Local/remote validation status is recorded separately;
no review approval, SOTA ranking or technology timing signoff is inferred.
