# Shared model tool: product scope and first milestone

Status: local design and implementation candidate; SOTA **NOT_ESTABLISHED**.
Starting implementation is `f7c7595`, documentation head `73c0305`, main base
`24fa8980`. Research snapshot accessed 2026-11-01. No external benchmark result
is inferred from a paper, README, internal speed ratio or a passing example.

## Current tool scope

The bounded request/FIFO projection and the
[complete-state clocked profile](clocked-model-execution.md) share the public
source-unit/compiler/runtime flow. The latter admits pure hierarchy with one
complete state witness and folds only proved idle ticks. Consumer algorithms
and their validation are separate from framework source. This tool change can
build, install and run its public non-CPU regressions independently.

The M1 assessment below is historical. General multi-owner projection and the
proposed mutable worker hierarchy remain future work; the complete-state
profile does not claim them. Independent review remains NOT_RUN after an
unavailable configured reviewer returned HTTP401 before producing an opinion.

## Users and competition boundary

Target processor/accelerator module authors and architecture exploration:
Python source, shared typed/effect IR, fast logical transactions, explicit
cycle timing and executable RTL, with replayable consistency evidence. This
is not a claim to replace all HLS, event simulators or complete SoC tools.

| Baseline, fixed research version | Relevant actual capability | Comparison boundary |
| --- | --- | --- |
| [PyMTL3 v3.1.17](https://github.com/pymtl/pymtl3/tree/c8b349f765d63a215dcd3def8a0c9e42d0bf8f9f) | Python multilevel FL/CL/RTL, method interfaces, modular passes, RTL translation/import and common testbench reuse. | The [2020 primary paper](https://www.csl.cornell.edu/~cbatten/pdfs/jiang-pymtl3-ieeemicro2020.pdf) describes manual refinement of FL into CL/RTL; that workflow is not evidence that all levels are automatically derived. Do not claim PyMTL lacks simulation, native imported RTL or composition. |
| [XLS bc53272b](https://github.com/google/xls/tree/bc53272bde483ed4d5ae7fced2e5bda2e2a5606f) | IR interpretation and LLVM JIT/AOT, proc/state/channel semantics, scheduled Block IR simulation and Verilog generation. | [Official execution documentation](https://google.github.io/xls/interpreters/) already presents multiple execution routes from common IR. It is a serious same-source baseline, not merely two emitters. DSLX rather than Python authoring; precise timing/handshake contracts must be aligned per task. |
| [CIRCT 5caf1e2f](https://github.com/llvm/circt/blob/5caf1e2fdc38ea61ab996c3ad3eb7ef25839ae0f/docs/Dialects/Arc.md) | Arc state-transfer functions and explicit sequential state; arcilator compiles lowered hardware to LLVM/native simulation. | Simulation of the same hardware is not automatically an architectural wait-folding strategy. Do not infer absence of other CIRCT pipelines from this Arc document. |
| [Verilator v5.048](https://github.com/verilator/verilator/tree/v5.048) | Compiles RTL into a native simulator; mature RTL simulation baseline. | Compare equivalent RTL/native host loops and semantics. C++ versus Verilog is a backend distinction, not a functional-versus-timing abstraction. |

Only PyMTL's documented workflow, XLS execution modes, Arc's stated purpose and
Verilator's version contract are assessed here. External runtime scores are
NOT_RUN until a versioned adapter is executed. Published speed claims are not
our measurements. Comparison revisions stay fixed during this milestone.

## Current evidence and largest gaps

| Capability | At start | M1 decision |
| --- | --- | --- |
| Same source to selected functional/timing entry | Measured for three bounded single-worker profiles | Retain contract and artifacts. |
| C++ workers1/2 and Icarus | Measured, raw/flags/state and FIFO timing oracles | Exercise the new composition only plus affected regressions. |
| Standard example gate | 13/13 with pinned Verilator5.048 | Reuse correct tool; no5.020 conclusions. |
| Worker hierarchy | Backend supports Method composition; projection rejects child instances | Add a proved resource-free deterministic owned module tree. |
| Independently buffered multiple issue workers | Unsupported | Deferred; requires queue graph/order and effect-ownership proof. |
| New user project/doctor/alignment/performance UX | No installed project commands | Add an installed project specification and commands, not a second compiler. |
| Native steady execution vs process/I/O timing | No qualified public benchmark | Separate measured scopes, retain raw repeats and independent correctness. |
| Physical/synthesizable RTL of projected stateful model | Not established by managed simulation | Report NOT_ESTABLISHED; do not label emit success synthesis. |
| Arbitrary time-dependent semantics, cancellation, domains, traps | Rejected/outside admission | Stay rejected; no silent approximation. |

## M1 scope decision after assessment

The proposed hierarchy changes a safety admission proof, not just a backend:
`ExecutionProjection.cpp:551` currently rejects child instances/calls, and
transitive snapshot write/guard/owner checks would be new obligations. The
required independent design-review request through the configured Broker route
failed HTTP401 before producing a review. No credentials were changed or retried.
Independent review is **NOT_RUN**, not self-review approval. Hierarchical
projection and role-dictionary shorthand below remain **DESIGN/NOT_IMPLEMENTED**.

The locally closed M1 is the explicitly permitted foundation: installed
init/doctor/verify/benchmark, supplied-golden alignment, correct completed-versus
requested receipts, repeated native batch mode with exact expected-packet
checking and explicit Release build configuration. Existing single-worker
projection admission/IR/runtime semantics are not expanded. Three project
variants exercise independent-result calculation, recurrence and FIFO
backpressure. These are usable engineering/measurement improvements, not
proof of composition or SOTA. Full hierarchy work is M2 after independent review.

## Proposed composition contract (pending review)

A packet/word processing component composes owned deterministic child Modules.
Each leaf has closed known finite state, total scalar computations, unconditional
callable guards and no autonomous/resource/observer effects. The root process
may call owned child Methods in source order; child state/effects join the
existing outer transaction. The complete instance/method graph must be finite,
acyclic and in the same clock/reset domain. No externs, reference aliases,
queues/memory inside the worker, hidden inputs, arbitrary type/static bindings
or cross-domain work are added. Read-only snapshots are proved transitively;
a nested mutating accessor cannot masquerade as a snapshot. Existing Method
summary and transaction lowering remain the only owners of scheduling/commit.

This is **atomic hierarchical worker composition** inside one issue transaction,
not an independently buffered multi-stage pipeline. The original ingress and
egress queues retain latency, II and finite-capacity backpressure. A blocked
result publication discards every child state proposal. Functional projection
calls this same hierarchy once per admitted request. No second functional body,
private driver, numeric shortcut or latency-zero rewrite is introduced.

A literal execution dictionary may omit roles with documented conventional
defaults. This is explicit opt-in declaration shorthand, not semantic inference
from arbitrary signal names; the published interface/body keeps the full typed
contract. Existing full dictionaries remain compatible. Unsupported declarations
fail with source locations. No schema relaxation can replace actual SSA/effect
proof.

An installed `init` creates a self-contained project (source, requests, independent
expected responses/state and project metadata); `doctor` reports exact compiler,
Runtime and simulator selection. `verify` selects both strategies through the
existing compile/link/emit/run path, compares raw publications, completion flags
and final state with user-owned expectations, and reports the first mismatch.
No result is manufactured from those expectations. Existing `run` remains usable.

A benchmark command reuses the same generated native driver/DUT. Raw records,
handshake and state observation are the normal route. Batched execution excludes
per-cycle stdout only in an explicit measurement mode, retaining actual response
fingerprints and final-state checks against independent expected data. Warm-up,
reset/construction, input parsing, native loop and output costs must be labelled.
A counter of requested samples must never be reported as completed samples.

## Acceptance and bounded scope

Items1-2 below are pending composition criteria, not M1 completion claims. The
foundation delivers3-4 for the bounded single-worker projects, with explicit
external/synthesis/scale NOT_RUN entries. Applicable CLI/adapter regression and
documentation checks in5 run; unchanged native IR/example gates are carried
forward as prior evidence, not rerun results.

1. Retain a pre-change legal hierarchical source rejection, then compile/link
   the unchanged source through both execution strategies after the proof change.
2. Independent nonzero outputs and child/final states, disabled/invalid effects,
   acceptance snapshots, FIFO maturity/capacity/II and backpressure recovery.
   Include one nested unsafe resource or mutating snapshot rejection with no
   final-artifact replacement. Core unknown/whole-transaction checks stay active.
3. Fresh project outside the repository, using only the installed prefix; native
   functional/timing plus actual Icarus RTL, independent expected data and error
   localization. No source checkout or private management log required.
4. Representative pure calculation, state dependency and backpressured composition
   evaluation entries; repeated native samples, raw values, median/IQR, build
   time/memory/artifact-size and size scaling. External tools may be NOT_RUN with
   an executable adapter and concrete reason; no unbounded install/build wait.
5. Affected native/CLI/source gates, pinned standard example gate if common IR
   changes, changed-file hooks and strict docs. No old full precision/ISA/SDK work.

## Evaluation protocol

Record tool revision/version, CPU/OS, compiler/optimization, workers, bit widths,
logical request sequence, states/reset, capacity/latency/II and sink schedule.
Functional transactions/s counts completed enabled requests; valid results are
separate. Timing cycles/s counts successful rising cycles, alongside completed
transactions. Never divide these unlike rates to claim a speedup.

Keep cold source/IR/emit/native-build wall time and maximum process RSS separate
from warm incremental time and execution. Identify the RSS measurement method
and child-process limitation. Report artifact bytes from actual inventories.
Native loop time, validation/fingerprint cost, parsing/setup/reset and external
process wall/I/O must be distinguished. At least five recorded samples after
warm-up, median and IQR/range, and correctness for every measured sample.
Small input sizes are latency measurements, not steady throughput evidence.

Compare external native-to-native only after semantic equivalence and the same
measurement boundary. A PyMTL Python-loop score can be reported as that mode's
score but cannot establish superiority over PyMTL native imported RTL. Missing
XLS/arcilator builds are NOT_RUN, not zero speed. Synthesis/area/fmax require a
separate physical, technology/tool-pinned experiment and are N/A in M1 unless
actually executed. No universal SOTA claim is admitted.

## Roadmap and migration

- M1: usable installed project, golden alignment and honest native batch/performance
  ledger over the existing bounded projection. No hierarchy/SOTA claim.
- M2: first the reviewed atomic owned hierarchy described above; then separately
  designed buffered finite worker chains, effect/commit and rejection routing,
  latency/II sweeps. Do not conflate those two capabilities.
- M3: stable trace/event and source-location schema, checkpoint/replay, qualified
  physical RTL synthesis and multiple external native adapters at matching scale.
- M4: larger processor/accelerator studies and externally reproducible results.
  Any leading result must name exact task, tool versions, metric and exclusions.

Project metadata is versioned and rejects unknown versions. Explicit role
contracts continue to work. The proposed shorthand would change authoring convenience, not
accepted timing semantics; it is not implemented in M1. Existing generated receipts are not silently treated
as newer measurement schemas. Local implementation is not independent review;
review identity, actual findings and unrun gates are recorded separately.

## M1 measured status

| Item | Status and actual boundary |
| --- | --- |
| Installed project, doctor, raw/state alignment | MEASURED: clean consumer outside the checkout, functional/timing × C++/Icarus; every published snapshot covered, first golden mismatch and bad input row/field localized. |
| Shared final IR for backend pair | MEASURED: verify with --rtl requires equal final artifacts per strategy, separately from golden checks. |
| Native batch/Release path | MEASURED: exact raw packets per batch plus final state, five samples after one warmup; internal O0 comparison only. |
| Independent cycle expectations | MEASURED for the initialized backpressure project: finite depth/maturity, stall/recovery and accepted snapshots; no arbitrary profile certification. |
| External PyMTL3.1.17 | Python reference ran; native import NOT_RUN because pkg-config did not find Verilator metadata. The reference is a combinational kernel, not an equivalent queue/state workload, so cross-tool ranking is N/A. |
| XLS/arcilator runtime performance | NOT_RUN: reference versions/docs fixed; matching workload/runtime adapters are future work. No full SDK builds attempted. |
| Stateless physical kernel probe | NEGATIVE EVIDENCE: compile/link rc0; physical emit rc1, root owner required. Synthesis/area/fmax NOT_ESTABLISHED, with no diagnostic suppression. |
| Hierarchy/shorthand | DESIGN ONLY: valid owned-child unit compiles, functional link still rejects ac.instance. No relaxed admission or after-fix claim. |
| Independent review | NOT_RUN: Broker request failed HTTP401 before review. |
| SOTA | NOT_ESTABLISHED: internal measurements and reference adapters do not establish a leading comparison. |
