# System compilation and simulation

`@system` is a current pyCircuit language construct for a closed simulation
composition. Authors describe the DUT, stimulus state, checks and observations
in Python. The compiler generates the GFSIM system and system Verilog, including
the executable simulation drivers. Authors do not write C++ drivers, RTL
benches, runtime JSON, or clock/reset/enable wiring.

## One compilation boundary

A system uses the same syntax capture, source-unit compilation, MLIR analysis,
interface publication, link closure and verified final hardware IR as a module.
Source capture never executes Python test code. A system's state and rules are
hardware, with the same old-state Work and atomic Xfer behavior as its DUT.

The current representation retains a verified system declaration role on the
shared module body and owning interface. The final system entry selects that
body. This role must survive serialization and match the provider body/header;
it cannot be forged by a filename or accepted as an unchecked decorator alias.
A source system is root-only and has no authored external arguments or results.
It may instantiate ordinary modules from independently compiled source units.

The existing MLIR binding and write analyses infer hidden clock/reset
connections and storage write enables. Their verified physical input ordinals
are preserved independently of a module's result representation. Emitters consume
this authority; they do not recognize clock names or counter recipes.

## Generated closure

| Artifact | Owner and purpose |
| --- | --- |
| Source units and published interfaces | Each Python source owns one reusable compilation unit. |
| Verified final IR | Link checks the explicit complete unit closure, types, effects and identities. |
| C++ modules and typed DUT | Existing source-owned code generation and shared GFSIM Runtime. |
| Generated native main | Shared SystemRunner, inferred control driving, finite runtime limits and source events. |
| Verilog modules and simulation top | Same final IR, standard hardware leaves and managed simulation lifecycle. |
| Generated CMake and manifest | Build a native executable or a Verilator executable without author scaffolding. |

`pycircuit run <example-directory> --target cpp|verilog --cycles N` composes the
existing build and public compile/link/emit commands. The example's small CMake
source list fixes the dependency order and root. The selected backend requires
only its own simulator; independent oracle tools are validation dependencies.
Both emitted directories remain independently buildable using the installed
Runtime package. Generated files remain outside the authored source tree.

## Execution and failure

The generated driver performs host Reset, then a finite number of low/high
sampling pairs. Each sampling epoch performs Work, whole-system checks and
one successful Xfer or a complete discard. Hardware expressions—including
write enables—remain untouched. Source fixture state supplies stimuli and
expected behavior; the runner invents neither data inputs nor golden answers.

The C++ path uses existing Runtime precheck and observation publication.
The Verilog path uses the existing standard leaves' managed prepare, commit,
discard and reset-prepare controls. Preparation snapshots old-state values,
source checks and observations. The simulation root freezes one whole-system
permission before any leaf commits. Discard preserves both state and clock
history. Failed epochs emit no source observations.

An ordinary gated clock is not a commit barrier: changing permission while the
clock is high could introduce an unintended edge. Generated simulation must
use the standard managed lifecycle, and normal design RTL retains its physical
clock semantics. Checks cannot be replaced by a fatal message after state has
already committed.

An ordinary module with reachable source assertions also requires the managed
lifecycle. Its original data, clock and reset ports remain present; emitted RTL
adds `pyc_phase`, `pyc_root_commit_ok` and `pyc_local_error`. The caller must
prepare the original input row, sample old-state outputs, freeze permission
only when the complete error signal is exactly zero, and then commit or discard.
Reset context comes from verified physical domain ordinals. Modules without
reachable assertions retain their physical clock interface. An ordinary module
does not gain a generated simulation main or the closed `@system` signature.
Ordinary-module RTL observations remain unsupported and diagnose explicitly.

Runtime failure is sticky until host `Reset()`. A physical reset input cannot
recover a failed execution. Host Reset clears state before the next sample;
synchronous hardware reset instead samples old state before committing reset.
Recovery tests therefore identify host-Reset segments and their independent
expectations separately from the original physical-reset history.

Observations use a shared occurrence layout for both backends. The system
publishes them only after a successful epoch, in deterministic instance/rule/
site order. C++ worker count must not change that order. Unsupported payloads
or independently driven domains fail explicitly rather than losing effects.
One authored log with several supported scalar operands produces one event
whose `values` array retains their order. A literal-only log has an empty array;
a report retains its scalar statistic. An entire inactive log emits nothing.
Partially active, incomplete or inconsistent operand groups reject before any
observations from that epoch are published. This grouping does not extend the
supported scalar widths or introduce four-state observation transport.

## Acceptance and example adaptation

Before this delivery is mergeable, a two-source stateful DUT/system must run as
generated C++ and generated Verilator simulation without handwritten harnesses.
Checks cover different widths/names, source-unit identity substitution,
system-as-child rejection, reset/retry, source observations and failure while
multiple modules propose writes. Both state and clock history must remain
unchanged after failure.

The user's adaptation scope is the original 93 design roots, including cases
previously moved to API tests. Each must be mapped to a current system closure;
deleting an uncovered row or wrapping one counter repeatedly is not adaptation.
Keep independent hardware oracles outside product lowering and generated default
drivers. Full coverage and long reference/mutation matrices remain in the
existing nightly entrypoints; a generated smoke run is not their replacement.

This design is an implementation contract, not evidence that all systems or
all examples already pass. Current run results and remaining failures belong
in candidate-bound verification records.
