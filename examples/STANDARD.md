# Example authoring and verification standard

Author designs as sequential Python functions and Rule bodies. Module variables
hold state; ACIR creates storage and control. Do not expose clock/reset, register
q/d terminals, DFF/DFFE bindings or proposal/commit wires in ordinary sources.
Use a scalar directly for one value. A Struct combines multiple related fields;
a one-field wrapper or singleton tuple adds no useful interface structure.
Physical primitive and nominal-identity compiler tests are separate conformance
fixtures, not patterns for ordinary examples or benchmark ports.

The [catalog](catalog.json) is the current source of truth for runnable examples
and API-owned coverage. The [navigation](README.md) is generated from that
catalog. Each listed example retains its own hardware behavior and independent
oracle; one example does not establish another example's behavior.

The current catalog contains 21 examples. The 54 retired source directories in
`tests/fixtures/historical_examples/` preserve comparison assets and old oracles;
their presence is not a catalog target or passing result. Do not add mechanisms
whose purpose is to recover a historical example count.

## Which designs belong in examples

An example teaches a current recommended authoring pattern or demonstrates a
useful complete design. Before porting an old example, identify that purpose
and check whether another example already demonstrates it. A historical folder
count is not a delivery target.

Prefer a small set of representative designs. Merge redundant syntax/type
variants, and keep compiler admission boundaries, exhaustive operator matrices,
malformed IR, and resource-limit stress in their existing API test owners.
Names and READMEs must describe current behavior: a fixed address projection
should not advertise parameterization, and ordinary Module code should not
advertise retired JIT or builder APIs.

Keep real architectural differences: independent interfaces, ordered local
Rules, nested cross-Module transactions, persistent aggregate state, and actual
memory/clock-domain behavior are not redundant merely because they use similar
Python syntax. A slow or failing test is not by itself a retirement reason.
Record the replacement and retained semantic coverage before removing an old
regression; retirement must not be reported as a passing run.

## Folder contents

Each current example needs its Python source, a short README explaining the
behavior and relevant boundaries, and the existing CMake integration. Prefer a
small `@system` scenario with expected-value assertions and compiler-generated
C++/RTL execution.

Add a custom `driver.cpp`, `rtl_tb.sv`, or `config.json` only when it tests a
specific boundary the generated System cannot exercise, such as explicit
independent clocks, host Discard/retry, or authored four-state stimulus. Do not
require three separately maintained testbenches for every small example.
Existing boundary oracles remain in place until an equivalent test owner is
identified and verified.

Use `GENERATED.md`/`GENERATED.json` when publishing generated-code excerpts;
refresh them from actual verified output. They are documentation samples, not
another compiled implementation. Complete generated artifacts stay in build
outputs. Long sweeps and stress matrices stay in the existing nightly/API
entrypoints; a tutorial's quick-start run need not repeat every compiler edge
case.

## Required compiler-stage deliverables

Each catalog example compiles its source units, links the explicit closure and
emits complete C++ and Verilog from the same final ACIR. Preserve source-unit
interfaces and the explicit link closure. Reuse the current build helpers and
API/example owners; do not add a parallel compilation route.

Bind generated excerpts to the exact source, compiler, runner and generated
files. MLIR verification proves the implemented invariants; behavioral
equivalence comes from the example's independent hardware oracle.

## Python style

Use `import pycircuit as ac`, `ac.bits[W]/ac.uN`, named structs and defaults,
ordinary variables and rules, and direct typed module calls. Group fields that
normally travel together to one downstream module in a named Struct, with one
Struct output connected directly to one Struct input. Prefer updating that shared
type as fields evolve instead of adding parallel arguments/results at every
module boundary. Avoid flattening and immediately rebuilding the same interface.

For independent interfaces or outputs routed separately, use `tuple[T0, T1, ...]`,
return them in order, and unpack the call directly. Each output may have its own
scalar or Struct type. Keep unrelated interfaces separate; do not impose a
Struct/NamedTuple wrapper or string-keyed mapping over all outputs. Follow the
[module output authoring contract](../docs/development/agent-frontend-guide.md#module-hierarchy-and-independent-outputs)
and [runnable example](multi_outputs/README.md). Use source values as values;
allocation comes from declared owners.

For ordinary designs, keep Python free of DFF/DFFE, clock/reset,
current/next/proposal and explicit instance binding. Physical primitive and
independent clock-domain examples retain explicit standard storage leaves and
physical pins to test those contracts. Their surrounding Module interfaces still
use typed results and direct calls; this exception is not a general state API.
Each static Module-call occurrence allocates a distinct child; aliases and reused
results keep the existing instance. Module calls describe hierarchy and data
dependencies, not extra cycles or commit priority. Do not introduce a helper
abstraction just to work around a compiler defect; report and repair the owning
generic flow.

Preserve each original numerical and timing contract. For example, a fixed-width
modular sum and an exact Integer sum are different; a data select with unknown
enable differs from a guarded DFFE update; two pipeline stages must remain two
stages. Original capacity, latency and configuration ranges are part of the task.

## Rule cohesion and interfaces

Agents must follow the [Rule authoring contract](../docs/development/agent-frontend-guide.md#rule-authoring-contract).
Each Rule should implement one cohesive function and its atomic effects. Keep
independent functions in separate Rules; preserve effects that must commit
together when decomposing an example. A Rule may be defined within its Module;
its locals remain combinational values, not persistent storage.

Use Module-style typed inputs and multi-output calls: tuple results with direct
unpacking for independent interfaces, and named Struct parameters/results for
fields belonging to the same interface.

Keep resource full/empty/ready and output acceptance guards in the compiler's
interface analysis and Rule lowering rather than repeating them in Rule bodies.
Keep algorithmic predicates explicit. Existing manual handshakes remain until
equivalent compiler behavior is implemented and verified; never silently remove
backpressure, alter atomicity or add registers to make an example look simpler.
Record unsupported syntax and implicit-guard gaps in #319 and the example's
migration notes. Documentation changes are not migration or execution evidence.

For ordinary queued Module interfaces, agents should use typed `put`/`get`
Rules (or meaningful domain names) and Queue `push`/`pop`, following the
[Queue-facing interface pattern](../docs/development/agent-frontend-guide.md#queue-facing-interfaces).
Do not expose FIFO readiness or wrap payloads in a new valid/ready protocol
merely to connect two source Modules. Keep required external protocol signals
at boundary adapters. A callable Rule is not automatically registered as
independent work; register only the intended autonomous Rules.

Ordinary Rule output availability is compiler generated. Do not add an authored
commit-valid field, a replacement zero or a holding register for a rejected Rule.
Only reached consumers require its payload to be available. Existing explicit
protocol fields, including raw queue `valid`/`ready`, keep their protocol meaning.
The cancelled Signal resource has no declaration/set/get authoring API.

## State declaration checks

Declare module-owned persistent variables directly. The compiler lowers them to
DFF/DFFE storage children using the ordinary module-instance interface. This
applies by declared type, including Bits, Enum, Struct and arithmetic data types
with defined storage representations, rather than only the `u1`–`u63` aliases.
Keep ordinary local computations and child outputs as value/wire bindings.

When reviewing an example, trace state declarations to their storage children,
check type/initialization/enable/reset preservation, and validate the claimed
source path with C++ and RTL. Existing Method-state restrictions are implementation
gaps to track, not a reason to replace the authoring contract with manual
register wiring. See the [state-declaration checklist](../docs/development/agent-frontend-guide.md#required-state-declaration-checks).

## Simplicity, reuse and efficiency

These are acceptance requirements alongside correctness. A passing run does not
by itself establish good authoring or efficient output. Execution receipts retain
their exact correctness scope; they must not be presented as broader evidence.

Group fields of a shared connection and compatible state in named structs; keep
independent interfaces separate. Define shared combinational logic once and reuse
it through the supported rule/module flow. Avoid long lists
of unrelated scalar arguments, copied arithmetic networks, unnecessary wrappers,
and manual expansion used merely to bypass missing frontend capability. Moving
duplicated code into another file is not simplification. Where the source cannot
express a useful abstraction, repair the generic frontend instead of recognizing
the example or adding a parallel route.

Preserve hardware behavior rather than incidental source representation. State
fields with identical clock, reset and enable behavior may share a struct owner;
fields with different enables must retain those distinctions, including unknown
control failures. Storage bits, capacity, pipeline boundaries and atomicity remain
meaningful checks; an old scalar-instance count is not automatically an invariant.

For substantive refactors, retain the independent behavioral regression and
compare source repetition, generated IR/code size, compile cost and runtime cost
where affected. State the workload and limitations of each measurement. Report
regressions and mixed results explicitly; fewer source lines or a successful
simulation does not prove faster compilation, simulation or better hardware.

## Oracles and evidence

Keep independently derived expected behavior for every retained contract. A
short teaching System may replace a duplicated long scenario after the original
physical, X/Z, reset, Discard and conservation coverage has a named API, Module
or historical-fixture owner. Do not count retirement as a pass or infer that an
archived long scenario now compiles. Changes to a typed DUT interface may update
field observation while preserving value/known/Z planes and the algorithm
oracle. Exercise workers 1 and 2 and RTL from the same final artifact when that
owner requires them; two-state parity is not four-state evidence.

The shared verifier writes `execution.json` and a successful `verification.json`
binding real inputs, generated files, runner and Work traces. Failed or stale
receipts cannot generate accepted-looking artifact guides. Candidate acceptance
still requires review and the root-specific original oracle.

After building and running an example, generate its guide from actual output:

```sh
python3 tools/example_catalog.py generated --example rob \
  --build /absolute/build/examples/rob
python3 tools/example_catalog.py navigation
```

Add a catalog row only after its generated DUT runs the intended behavior, all
required checks pass and the receipt is bound to the tested source.

Long retained tests keep their hardware frequency, depth and latency. Use Release
for both Runtime and the generated DUT, procedural finite stimulus and assertions
that remain active with NDEBUG. Sparse WORK trace rows are checkpoints, not the
number of executed or checked epochs; verify the full epoch/edge totals inside
the independent driver and distinguish those counts in the README.

The existing CMake helper accepts TIMEOUT_SECONDS as a positive integer for each
verification subprocess (default180). Choose it from measured full-DUT pilots
with margin. The verifier records duration/budget and preserves partial logs plus
a failed execution record on timeout, while removing its success receipt. A pilot
prefix does not accept a historical design or replace its full timing test.

## Test ownership and tiers

`tools/run_examples.sh --tier gate|nightly` owns example build and
behavior acceptance through the shared CMake helper and verifier. Gate runs the
seven smoke designs listed in `examples/CMakeLists.txt`; nightly runs
the complete 21-example registered set. Long stimuli for current examples stay
intact in nightly; archived long benches remain outside that pass claim. API
contract, negative-admission and compiler analysis fixtures belong under `tests/`
and run through `tools/run_api_tests.sh`, independently of example acceptance.

## Sampling and runner configuration

Native `sample()` exposes the last successful Work snapshot; Xfer commits state
without recomputing that snapshot. RTL testbenches must compare the equivalent
settled precommit observation, then advance the physical clock. A continuous RTL
output observed after the edge is a different observation, not evidence of an
extra native cycle. Keep the independent expected values and compare matching
phases; never shift traces to hide an actual latency error.

Runner JSON uses `"schema": "pycircuit-model-config"` and `"version": "1"`.
The version is a JSON string, not numeric `1`. Include a finite `max_ticks`.

## Simulation build profile

The standard example gate and `pycircuit run` build generated simulation code
with CMake Release optimization. Use the same optimized profile for complete
cycle workloads and timing comparisons. Keep all source checks and the declared
current scenario contract. Changing a tutorial cycle count requires explicit
new expectations plus preservation of any displaced long regression under its
real owner; compiler optimization alone is not permission to reduce coverage.
Manual generated-code builds may select Debug when debugging.
