# Installed model projects and measurements

This local M1 adds project tooling over the existing source-unit compiler and
Runtime. It does **not** expand projection admission: one single-domain finite
request/response worker remains the supported slice. See the
[product plan](model-tool-product-plan.md) for fixed competitor references,
capability status and the pending hierarchy design. SOTA is NOT_ESTABLISHED.

## Install and create a project

Build/install from the candidate through the repository's existing CompilerDev
workflow (LLVM/MLIR22.1.8). An existing matching build can be installed again;
no new SDK is required. Select its prefix and put its bin directory on PATH.
Generated C++ consumers need Runtime and a C++20 compiler, not LLVM discovery.

```sh
export PYC_TOOLCHAIN_ROOT=/path/to/candidate/install
export PATH="$PYC_TOOLCHAIN_ROOT/bin:$PATH"
pycircuit doctor --rtl
pycircuit init telemetry --template backpressure
pycircuit verify telemetry --build-dir .pycircuit_out/telemetry-check --rtl
pycircuit benchmark telemetry --output .pycircuit_out/telemetry-measurement
```

Run these commands from a parent directory; build/measurement directories must
be outside the project source directory. init requires a new directory and
preserves existing files. It writes a source model, ordinary CMake entry,
requests.json, independent expected.json, project.json and README. No second
functional source or private host driver is required. The explicit current
execution-role dictionary is generated for the user; role inference/shorthand
and hierarchical worker projection are not implemented by this milestone.

`compute` produces independent normalized-word results; `stateful` adds a
wrapping checksum dependency; `backpressure` retains that calculation with a
finite result queue and delayed consumer. Six input records contain one inactive, one
disabled, three valid and one invalid request. Four accepted requests complete,
including the invalid; exactly three valid payloads are published. The recurrent
variants yield4811,28168,27912 and final accepted/total/completed/rejected
4/27912/4/1. The compute variant yields4811,23357,65280. All values are raw u16.

Doctor reports selected paths and available tool versions, not a certificate
that an arbitrary source is admitted. Its native prerequisites include the
installed source tools, Runtime package/archive, CMake, Ninja and C++ compiler;
--rtl also requires Icarus/vvp. Standard example gates separately require the
repository-pinned Verilator5.048. Missing optional tools remain visible.

## Golden alignment and diagnostics

Project metadata has the closed schema `pycircuit-model-project-v1`: qualified
model name, relative input/expectation paths, positive timing cycles and
nonnegative consumer_start. Unknown schema versions and paths outside the
project fail explicitly. Source authoring and owner/clock/effect errors retain
native source locations. Raw input range errors identify the input row and field.

Expected data contains the complete functional completion packets (including
invalid flags), valid publications and every published final snapshot. Known
raw expected response values may be integers or width-exact bit/hex strings.
No expected value is assigned to the DUT. verify compiles/links/emits and runs
both strategies, adding both Icarus views with --rtl. It compares supplied golden
packets/order and final state and stops at the first mismatch, naming its index,
expected/actual values and trace. Child build/execution failures include their
log path and native diagnostic tail. verification.json remains failed on error.

This is golden validation plus cross-view consistency, not a formal proof.
Independent cycle-level expectations for the delayed consumer, FIFO maturity,
capacity and recovery live in tests/system/test_model_project.py. Functional and
timing views need not have the same waveforms. A failed stream's rising_samples
counts published successful records; requested_rising_samples records its limit.
A negative stream with two records is not a completed24-cycle run.
Input-driven receipts now identify `pycircuit-model-run-v2`. Legacy unversioned
receipts retain their historical requested-count meaning; do not reinterpret
them as completed-sample evidence. Success counts remain compatible.

## Repeated performance runs

The benchmark first performs cold compile/link/emit/native-build and golden
verification, repeats the warm pipeline, then reuses those native executables.
The normal run adapter is explicitly built as Release (CMake's -O3/-DNDEBUG on
the measured GCC toolchain). Runtime/source checks remain active; required
unknown/inactive tests also run under that setting.

Default samples are5 after1 warm-up, at1/32/256 reset batches. Every completion
in every timed batch is compared exactly against the independently supplied raw
packet, with a diagnostic fingerprint as well. Every final state is checked.
The summary mode removes per-cycle stdout formatting, not DUT work, checks,
handshake or sampling. Native loop time includes input-bit decoding, execution,
controls, exact response checks and fingerprinting; excludes construction/reset
and final summary output. Process wall time additionally includes those costs
and input parsing. Initial/reset overhead has not disappeared from the report.

Report raw JSONL batches, per-sample loop/process times, median/IQR/min/max,
source/IR/link/emit and C++ build phase timing, generated artifact/executable
sizes and GNU-time maximum-process RSS. Cold and warm builds currently have one
observation each, not a statistically qualified compile-time result. If GNU time
is absent, RSS is null. Batch-volume scaling is not module-count scaling. Shared
host, unpinned CPU and short-batch noise limit interpretation.

Functional rate counts completed enabled requests including invalid completion;
valid payload count is separate. Timing rate counts successful physical cycles.
These different units are never divided into a speedup. The same native adapter
also supports a measured internal O0/Release comparison; that is an internal
compiler-option result, not external superiority.

## Boundaries and migration

Existing compile/link/emit/run APIs and full execution-role dictionaries remain
supported. Project and benchmark schemas are versioned; old generated-model
receipts are not silently promoted to a new measurement schema. Existing
single-worker source can be adopted by supplying project.json and independent
expectations; source/provider closure remains declared in its normal CMake file.

Actual new RTL checks use Icarus. A separate stateless XOR physical-export probe
compiled/linked but physical emit rejected it with `physical RTL requires a root
owner`; no synthesizable artifact or area/fmax result was produced. Managed RTL
simulation is not synthesis evidence. General physical-export, hierarchical or
independently buffered worker chains, architectural timers, cancellation,
cross-domain models and trapping workers are not established here.

Independent design review was requested through the configured Broker route but
failed HTTP401 before review; credentials were not changed/retried. No independent
review or hierarchy approval is claimed. The pending design's source evidence
is a valid hierarchical unit that still rejects at projection on ac.instance.
The foundation commands and batch optimization do not bypass that rejection.
