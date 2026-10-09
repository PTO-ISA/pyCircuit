# Example authoring and verification standard

The [catalog](catalog.json) is the current source of truth for runnable examples
and API-owned coverage. The [navigation](README.md) is generated from that
catalog. Each listed example retains its own hardware behavior and independent
oracle; one example does not establish another example's behavior.

## Folder contents

Use one flat `examples/<design>/` folder containing:

- `<design>.py` (and explicitly listed same-design source units when required);
- `README.md`: intent, original source/oracle provenance, inputs/results, state,
  timing, initialization/reset/hold/failure behavior, build commands and limits;
- `driver.cpp`, `rtl_tb.sv`, `config.json`: finite independent behavioral tests;
- `CMakeLists.txt` invoking the installed `PycircuitExamples.cmake` helper;
- `GENERATED.md` and `GENERATED.json`: actual generated excerpts and their
  verification/source/artifact fingerprints.

Build directories, native executables and complete generated source trees stay
outside the checkout. Generated guides are documentation samples, not compiled
sources or an alternate model implementation.

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
ordinary variables and rules, and direct typed module calls. Module inputs and
struct fields are declared once; return a typed value, not a string-keyed port
mapping. Use source values as values; allocation comes from declared owners.

Keep Python free of DFF/DFFE, clock/reset, current/next/proposal and explicit
instance binding. Module calls describe a static hierarchy and data dependencies,
not extra cycles or commit priority. Do not introduce a helper abstraction just
to work around a compiler defect; report and repair the owning generic flow.

Preserve each original numerical and timing contract. For example, a fixed-width
modular sum and an exact Integer sum are different; a data select with unknown
enable differs from a guarded DFFE update; two pipeline stages must remain two
stages. Original capacity, latency and configuration ranges are part of the task.

## Simplicity, reuse and efficiency

These are acceptance requirements alongside correctness. A passing run does not
by itself establish good authoring or efficient output. Execution receipts retain
their exact correctness scope; they must not be presented as broader evidence.

Group related interfaces and state in named structs. Define shared combinational
logic once and reuse it through the supported rule/module flow. Avoid long lists
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

Keep the original stimulus and independently derived expected behavior. Changes
to a typed DUT interface may update field observation, preserving value/known/Z
planes, without altering the algorithm oracle. Exercise workers 1 and 2 and RTL
from the same final artifact. Retain original X/Z and phase-specific checks with
an appropriate simulator; two-state parity is not four-state evidence.

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

Long tests keep the original hardware frequency, depth and latency. Use Release
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

`flows/scripts/run_examples.sh --tier gate|nightly` owns example build and
behavior acceptance through the shared CMake helper and verifier. Gate runs the
six smoke designs listed in `examples/CMakeLists.txt`; nightly runs
the complete registered set. Long stimuli stay intact in nightly. API contract,
negative-admission and compiler analysis fixtures belong under `tests/` and run
through `flows/scripts/run_api_tests.sh`, independently of example acceptance.

## Sampling and runner configuration

Native `sample()` exposes the last successful Work snapshot; Xfer commits state
without recomputing that snapshot. RTL testbenches must compare the equivalent
settled precommit observation, then advance the physical clock. A continuous RTL
output observed after the edge is a different observation, not evidence of an
extra native cycle. Keep the independent expected values and compare matching
phases; never shift traces to hide an actual latency error.

Runner JSON uses `"schema": "pycircuit-model-config"` and `"version": "1"`.
The version is a JSON string, not numeric `1`. Include a finite `max_ticks`.
