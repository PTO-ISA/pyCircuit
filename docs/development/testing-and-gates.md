# Testing and gates

Gate claims must match the exact candidate and active pyCircuit route.
Record raw outcomes and candidate identity under the ignored local directory
`docs/gates/logs/<run-id>/`; CI uploads these files as workflow artifacts.
A command, test file or historical acceptance entry is not a passing result.

## Short native CI

`G1: Native Transactional Gates` builds and installs the compiler, runs the
native transactional runtime foundation and the readable `transactional-*`
end-to-end scenarios. Physical C++ workers 1/2 and RTL compare against independent
histories; t-gfsim reuses supported State/control/call designs. Saved Linked and
Routed continuation and a Runtime-only generated consumer remain covered.

See [transactional test scenarios](transactional-test-scenarios.md) for the six
groups, positive supplements, backend support and coverage migration. Issue #361
rejection, malformed-IR and implementation-structure tests are deliberately
removed; their removal does not mean those cases passed. Finite System reset
schedules run in the State and Data groups; post-failure generated storage
observation remains an explicit gap. The desired required-check list includes
G1; applying branch protection on GitHub is a separate repository-management
action.

## API tests and examples are separate

Foundational reliability covers declared types and widths, combinational
expressions, state/current-next timing, Module/Rule/Method composition,
Queue/Memory behavior, reset and four-state values, failure atomicity, source-unit
authority, and matching C++/RTL results. Changes to these contracts must retain
their existing numerical, timing, rejection and rollback oracles. Migrating an
obsolete source spelling is separate from repairing a compiler failure.

High-level simulation, incremental evaluation and visualization are separate
extensions. General parameterization and Module families are authoring-capacity
gaps; passing a smaller supported design does not establish those capabilities.
Large retained regressions that exceed an analysis limit remain failures or
explicitly unverified cases, not evidence that an algorithm, width or scenario
may be reduced to obtain a pass. Use the existing gates below for both layers;
do not introduce a second test runner or claim full nightly from a focused set.

Use two entry points over the existing CTest, lit, pytest and example verifier:

```bash
bash tools/run_api_tests.sh --tier gate
bash tools/run_examples.sh --tier gate
# Full registered coverage, including the gate subset:
bash tools/run_api_tests.sh --tier nightly
bash tools/run_examples.sh --tier nightly
```

For an existing nondefault build/install pair, export both paths before running
these scripts (the examples and package tests use the installed toolchain):

```bash
export PYC_BUILD_DIR=/absolute/path/to/build
export PYC_TOOLCHAIN_ROOT=/absolute/path/to/install
cmake --build "$PYC_BUILD_DIR" --parallel 4
cmake --install "$PYC_BUILD_DIR" --prefix "$PYC_TOOLCHAIN_ROOT"
bash tools/run_api_tests.sh --tier gate
```

Both default to `gate`; `--list` reports the CTest selection without building or
claiming a pass. Set `PYC_BUILD_DIR` to this checkout's CompilerDev testing build
and `PYC_TOOLCHAIN_ROOT` to its installation. Reinstall after changing compiler,
Runtime or shared example helpers. `PYC_PYTHON_EXECUTABLE` selects Python for
package tests and receipts. No new native executable is needed for this split.

| Entry point | Gate | Additional nightly coverage |
| --- | --- | --- |
| API/framework | Native API/runtime GTests; IR/pass lit; source interface/check diagnostics and output protection | Remaining source/codegen matrices, native source-check execution, Runtime/CompilerDev install, public emit/source maps, incremental builds, relocation, publication recovery and presets |
| Examples | The explicit `_gate_examples` list in `examples/CMakeLists.txt`, including `resource_composition`; each compile/link/dual emit and workers 1/2/RTL | Every registered example, including full-duration `digital_clock` and BF16 checks |

The example registry is explicit. Gate configures only its selected smoke designs,
so it does not compile the full nightly set first. Direct aggregate CMake builds
default to the full `nightly` registry. Standalone example builds remain supported.
Examples demonstrate complete designs; API fixtures test compiler/runtime
contracts and rejection boundaries. An example pass does not substitute for API
coverage of unsupported historical designs.

`check-pycircuit` runs native/lit API tests only, selecting the cached
`PYC_TEST_TIER` (`gate` by default). The API script explicitly sets this on every
invocation and additionally runs its explicitly listed public-flow/package tests for nightly.
Uncovered historical API drafts have been removed. Retained semantic tests and
independent oracles remain in their existing owners. CTest labels `api`
and `examples` describe ownership; `gate` and `nightly` describe selection. Gate
entries also carry `nightly`. Complementary lit filters partition the inventory
without duplicating cases. Source-check capacity limits stay separate from the
long execution test. Python-only unit/hygiene/docs gates remain independent CI
checks and do not require the compiler toolchain.

The constant Table field-plane CodeGen regression is part of the API gate. It
checks native execution and four-state RTL, and requires Icarus Verilog
(`iverilog` and `vvp`) in addition to the native test tools. CI provides the
existing pinned Icarus toolchain through `setup-native-test-tools`.

Aggregate branch selection additionally checks field/row availability before
conjunction, sampled Rule fire, known and unknown selectors, and independent
source-proof mutations. The existing external-pop nightly owner retains the
complete one/two-instance allocation-pool reproducer: 16 state rows, two argmin
queries and five queues. It independently publishes the source units and checks
C++ workers 1/2 and RTL observations without reducing the pool or raising analysis
limits. This retained source workload still requires transactional frontend
migration; its registration is not a passing result. Generic Integer-state and
nested-Table compatibility remain separate regressions; a compact join does not
imply new source type admission.

Each executed entry point records its selected CTest names and count under
`docs/gates/logs/<run-id>/`, with distinct category/tier summary filenames.
A failed or empty run cannot publish a pass; `--list` publishes no pass summary.
Nightly and release run each category once with `--tier nightly`. A focused gate
result must not be reported as a full nightly result.

## Oracle ownership and scheduled coverage

User direction (2026-10-07): keep independent oracle models and framework
coverage outside hardware design sources. `tests/compiler/oracles/` contains
reference models/checkers; DUT fixtures describe hardware, and existing lit
owners retain public compile/link/emit, generated-model builds and drivers.
Do not add a second test runner or product compilation path.

Queue reference models are separated from `queue-source-vectors.py` serialization.
The existing nightly `Source/queue-source.test` adds `--oracle-checks` and reuses
its generated artifacts for the existing independent checkers. Reorder's causal
checks may not be skipped while reporting complete success. Generated native
observation coverage also belongs to the existing nightly filter; inexpensive
Runtime/source diagnostic coverage remains in gate.

Long coverage, differential/reference-model and mutation matrices run when
nightly is requested or scheduled. Ordinary authoring/review does not require
running them immediately. `examples/CMakeLists.txt` defines the current example
selection; the catalog describes each design's purpose and owner. Historical
fixtures outside that selection remain only where their capability or active
consumer needs them. Retirement is not a pass or proof of equivalent coverage.
Record each actual result and remaining NOT RUN case; focused passes do not
establish a full nightly result.

## Contract under validation

The public workflow is one source per `pycircuit compile`, explicit unit
closure through `pycircuit link`, and `pycircuit emit` to either backend from
the verified final artifact. The [transactional language profile](../reference/language.md#transactional-frontend-replacement)
defines current admission. Tests for retained legacy capabilities remain migration
requirements until revalidated; their registration alone is not a support claim.
Typed C++ DUTs and host-driven runners use Work/Xfer sampling epochs. Unsupported
sources must reject without fallback dispatch.

## Focused validation map

| Change | Evidence to collect |
| --- | --- |
| Capture/source semantics | Accepted module/rule fixtures, source provenance, range/type rejection, autonomous declarations and explicit global-template bindings, current/next and child-state identity |
| Unit compile and link | One producer per source, published interface use, complete closure, duplicate/missing/mismatched unit rejection, verified final artifact |
| Saved anchors | Exact schema/spec/body/configuration identities, managed and portable snapshots, source removal and relocation, native direct rejection, explicit refinement, unchanged dual-target execution and failure rollback |
| C++ emit | Source-owned module groups and `pycircuit_modules` CMake target, bound root arguments, Runtime-only consumer build and explicit test drivers |
| Verilog emit | Same final artifact, source-owned RTL/map inventory, unsupported configuration rejection, target output preservation |
| Runtime/package | Runtime-only install without LLVM discovery; CompilerDev install with LLVM/MLIR 22.1.8; exported target and external consumer smoke |
| Hard break | Source, CLI, CMake, package and installed-payload scans show no active retired frontend/compiler/fallback |
| Documentation | Active docs agree on profile, command path, unsupported contracts and current test selection |

## Retained verification responsibilities

The owners below describe the behavioral and IR obligations of existing tests,
including legacy cases awaiting migration. They do not extend the current
transactional admission profile or establish passing execution on a new candidate.
Preserve independent expectations when migrating these cases; obsolete internal
mechanism checks may be removed with explicit coverage accounting.

Source interface extraction has a standalone MLIR boundary:
`pycircuit-opt --ac-extract-source-interface`. Its direct tests cover
all-definition inference before replacement, exact signatures/metadata,
Table/Collection dependencies, temporal Q cuts and rollback after input,
analysis or staged-interface failure. Shared structural validators are tested
separately from provider/canonical-builtin authority. Keep owning-summary
tampering and public failed-replacement oracles; a range failure in the importer
does not prove failure originating in the pass.

Record optimization has a separate native gate for declaration-based forwarding,
exact result types, dominance, source metadata, untouched scalar/effect operations,
all-module preflight and dependency equality. Public-flow tests check saved
snapshots and four-state transport. Existing defaults, branches, state lifecycle
and provider-authority tests remain required; smaller IR does not prove these
semantics. The common-pass smoke test exercises explicitly registered upstream
canonicalization and CSE on func/arith IR, without adding either to the source
pipeline. Wide-fixture measurements use unchanged source/testbench bytes and
report retained size separately from transient importer cost.

Behavioral-source validation includes the generated ROB runner and a separate
eight-lane table fixture with different field widths and initial values. Preserve
sequential assignment, value-snapshot versus owner binding, alias rejection,
full-width index proofs, modular arithmetic, no-write clock sampling, failure
without a successful sample, and discard/retry checks. Packed struct source
tests verify current schema extraction and authority under field-type, nominal
identity and owner tampering. No new IR interface is required by this slice.

The defaults/composition gate additionally checks static defaults even when
unused or overridden, nested zero/default distinctions, immutable field updates,
typed struct results, and hidden-domain propagation through stateless parents.
Separate child instances, result fanout and sequential data dependencies use
independent native/RTL oracles. Preserve the ROB's original 569-sample stimulus
and scoreboard when changing its output transport to a packed struct; only the
test's observation access should change.

The grant extension additionally distinguishes pending capture plans from final
SSA proof, checks every pair across all source modules, and verifies complementary
and explicit priority grants in both registration orders. Native tests cover
unknown-grant failure and whole-state/clock discard; Icarus grant tests cover X/Z
payload transport, not a separate proof of RTL unknown-enable rollback. Private
resource caps and source-level cycles fail closed with publication protection.

The `multi-rule-writes.test` gate checks the registered capture-analysis pass,
write-intent conflicts, natural void registrations, nested-field merging, old-Q
reads, rule-order independence and whole-system data/clock discard. It also
preserves single-rule unknown-control semantics. Public native workers 1/2 and
RTL share the final IR; native/Icarus checks provide four-state evidence.

Run the narrowest check covering the change. Preserve supported behavior and
rejection oracles. Incomplete historical examples/API drafts were removed at
the delivery cutoff; this is a scope reduction, not a passing test result.

`examples/CMakeLists.txt` registers executable designs, documented in
`examples/catalog.json`. The installed helper compiles each source, links its
closure and emits both targets. Per-design assertions and worker/RTL comparison
validate hardware behavior. Use CTest with `--no-tests=error`; an empty registry
is not execution evidence. No example or test may require local migration logs.

## Build profiles

The root build exposes `PYC_BUILD_COMPILER_DEV`, `PYC_BUILD_TESTING`, and
`PYC_BUILD_RUNTIME_LIB`. A Runtime-only package is configured with compiler dev
off, runtime on, and testing off; it must not discover LLVM or MLIR. The
CompilerDev profile requires exactly LLVM/MLIR 22.1.8. Runtime consumers use
`find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)` and
`pycircuit::pyc6_runtime`.

When Runtime is configured, `check-pycircuit` builds its archive before running
the source gates that compile and execute generated DUTs. A fresh build directory
uses the same gate command; it does not require a separate Runtime build first.

## Reporting

For each gate, state the exact command, exit status, output/evidence path,
candidate revision or content binding, and any skipped or unavailable checks.
Do not infer implementation or complete coverage from documentation, one
passing lane or an older candidate. See the [source-unit workflow](source-unit-workflow.md)
for build ownership and the [language reference](../reference/language.md) for
unsupported capabilities.

## Routed anchor checks

The native `ACIRRoutedAnchorTests` executable separates local operation checks
from standalone graph legality, including fresh-context parse/verify, state
ownership and temporal cuts, metadata mutations and ordinary extern availability.
Existing transaction-effect tests retain original-source authentication and add
independent routed-to-physical mutation checks. A graph-only pass does not prove
either transformation preserved its input.

Run focused native tests before the API and example gates. Hardware-only budget
regressions, failed-stage/output preservation, shared-definition state isolation
and generated C++/four-state RTL execution remain required. The native routed
link target does not establish high-level emission or public anchor packaging.
