# pyCircuit agent instructions

pyCircuit is a hardware design language. The source-unit cutover retired the old compilation routes;
that bounded cutover does not mean the intended hardware language is complete.
Active product behavior is governed by the approved migration contracts and
the accepted implementation. Do not treat this file or any migration proposal as evidence that
implementation or tests have passed.

## Read first

- `docs/development/agent-frontend-guide.md`
- `docs/reference/language.md`
- `docs/development/contributing-workflow.md`
- `docs/development/testing-and-gates.md`
- `docs/development/review-and-merge.md`
- `docs/development/source-unit-workflow.md`
- `docs/rfcs/contracts/approvals/` for exact approved migration contracts
- `docs/development/project-governance.md` for bounded ownership and review expectations

## Hardware semantics and implementation coverage

Source modules are ordinary typed `@module` functions with nested stateless
`@rule` functions and explicit registrations. Structural calls create separate
instances; standard storage leaves own state. The current route supports typed
root ports, finite Integer/Boolean values, proven arithmetic and immutable local
SSA wires. Drivers supply explicit clock/reset levels, including independently
driven clock pins. Existing keyword-only defaults and standard-leaf type arguments
do not establish arbitrary parameter-dependent elaboration. Each source compiles
independently to a published unit; link takes the complete explicit closure.
C++ and Verilog emit from the same verified final artifact. Read the language
reference for exact current admission and limitations; the earlier portless
profile remains historical evidence.

Python capture does not execute the design. MLIR owns source resolution,
semantic checks, interface/effect derivation, and hardware lowering. Work reads
old Q; successful whole-system checking precedes Xfer. Pure local names share
combinational values without storage or an assignment-time sample. Source
`nonlocal` proposal syntax belongs to the retired source model.

Unsupported uses must fail closed with a diagnostic. Broader system authoring, source
collection coverage, additional FIFO/backpressure policies, wide instrumentation,
automatic domain scheduling/CDC, general arithmetic runtime checks and a DUT
shared-library port C ABI remain unfinished. Standard memory leaves do not close
historical disabled-read lifetime requirements. Host four-state I/O evidence
does not establish every source four-state constructor. Do not fill gaps through
retired routes or claim missing multiwriter/MayOverlap coverage from typed leaves.

## Hard break and build contract

The active product route is `pycircuit compile`, `pycircuit link`, and
`pycircuit emit --target cpp|verilog`. There is no fallback or compatibility
mode for CycleAwareSignal/JIT, structural builders, Agentic Circuit/QueueGraph,
`acc.py`, `acc`, or `pycc`.

The CMake build options are `PYC_BUILD_COMPILER_DEV`, `PYC_BUILD_TESTING`, and
`PYC_BUILD_RUNTIME_LIB`. Runtime-only consumers use
`find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)` and
`pycircuit::pyc6_runtime`; that component must not require LLVM. CompilerDev
requires exact LLVM/MLIR 22.1.8. Generated module CMake builds `pycircuit_modules`
against Runtime. Host drivers use typed `pyc_dut` and the shared SystemRunner,
own inputs and explicit clock/reset levels, and supply a finite runner limit.
The former generated `pycircuit_system`/`libpycircuit_dut` profile is historical;
it does not establish a current port C ABI.

Keep complete consumer designs, consumer testbenches, ISA decoders,
model-comparison scripts, consumer schemas, and adapters in their owning
repositories. Framework semantics stay design-neutral.

## Development work

Follow `docs/development/project-governance.md` and the source-unit workflow for
bounded ownership, approved contracts, independent tests/reviews, candidate
identity, and evidence. Do not modify frozen approval proposals or milestone
status as part of implementation/documentation work unless that file is
explicitly assigned.

For any feature or bug fix, identify affected decisions/contracts and the
smallest evidence needed. Build from this checkout; do not copy toolchains,
shared libraries, or generated outputs from another worktree. Keep generated
files and temporary artifacts out of the source tree. Update active behavior
docs with behavior changes.

## Hardware design and task boundaries

### Authoring and verification boundary (user direction, 2026-10-07)

- Keep one Python capture -> MLIR analysis/lowering -> verified common IR ->
  C++/Verilog flow. Improve existing owners; do not add another authoring
  profile, concept layer, compiler, runner or protocol to work around migration
  difficulty. pyCircuit remains a hardware programming language and compiler,
  not an arbitrary Python interpreter or a migration-management product.
- Keep capture/frontend thin. Put type, dependency, effect and hardware
  semantic inference in the existing MLIR analyses and passes. Remove reviewed
  shims, fixture-specific product rules and redundant concepts by repairing
  their existing frontend/IR/codegen owners; do not replace them with a wrapper.
  Real hardware regression fixtures remain test assets, never compiler rules.
- Keep hardware design sources focused on their algorithm, state and connections.
  Place migration bookkeeping, framework coverage and independent oracle models
  outside DUT sources in their existing documentation/test ownership. Oracle code
  must never supply DUT results or enter product lowering/codegen.
- Reuse existing API/example gate and nightly entrypoints. Put long coverage,
  reference-model and mutation matrices in nightly without shrinking scenarios.
  Separate validation scheduling from ordinary implementation/review work; when
  the user defers coverage/oracle runs, do not launch them or claim them passed.
- Preserve compiler-stage deliverables for retained supported roots, including
  API-owned roots: source-import ACIR, transformed ACIR, and C++/Verilog from
  the same final IR. Bind retained artifacts to the exact candidate through
  existing manifests. Excerpts and accepted counts are not stage verification.
- Keep repairs bounded. Review and fix concrete defects in the existing flow;
  a proposal for a new surface is not authorization to implement it. Preserve
  hardware timing, ownership, type authority and whole-system atomicity.

- pyCircuit is a hardware design language. Designs, modules, register references,
  stateless rules and test systems are the product concepts. A selected test
  fixture or an unfinished implementation slice is not the language definition.
- **NO HARDCODE:** do not use an example's name, width, initial value, increment,
  mask, node count, statement layout or number of ports to decide product
  semantics or admission. Derive behavior from declared types, actual SSA,
  effects, register ownership and approved operation semantics. Hardware source
  constants, independently derived oracle values and fixed primitive contracts
  are legitimate; scenario recognition as a compiler rule is not.
- **NO SHIM:** no compatibility aliases, old-route fallback, parallel semantic
  compiler, backend-only semantic patch, or adapter that bypasses common IR
  inference/verification. Rename or replace the owning implementation and its
  callers together. A temporary workaround needs removal before acceptance;
  calling it internal does not exempt it.
- For a complex change, a real `architect` agent establishes the design and an
  independent decomposition agent splits it into small tasks before execution.
  Each task fixes its inputs, exclusive files, dependencies, expected hardware
  behavior, minimal gates and removal scope. Executors implement those tasks;
  they do not redesign the framework while chasing a failing test.
- PM integrates shared registries and CMake. Implementation, independent tests
  and review use separate instances. Default implementation and code-review
  model is `gpt-6.1-sol`; record the actual role/model/effort. Use the real
  architect preset for architecture, not another role described as architect.
- Remove tests that freeze incidental recipes or duplicate another gate.
  Preserve meaningful ownership, type/range, old-Q, Xfer/hold/discard/reset,
  zero-commit-on-failure, source-unit and output-protection oracles. Never weaken
  hardware semantics to make an example pass, or claim broad migration from
  one or two small examples.

## Verification and reporting

Use the narrowest relevant gate, then the applicable candidate acceptance
checks. Bind semantic or decision-bearing evidence under
`docs/gates/logs/<run-id>/` to exact candidate content and commands. Report
commands, exit status, evidence paths, skipped checks, and remaining callers.
A fixture or named test is not a passing result. A documentation-only update
must not claim implementation or verification from documentation alone.

### Delivery cutoff (user direction, 2026-10-07)

The migration expansion is closed. Remove uncovered historical examples and API
test drafts instead of extending the refactor to complete them. Keep supported
semantic tests and independent oracles, with long coverage in the existing
nightly entrypoints. The current catalog is `examples/catalog.json`. Removed
coverage is not a passing result. Use descriptive names, not migration milestone
or task codes, in active code and documentation. Logs, migration work packets
and review transcripts are local ignored artifacts; tests must not read them.
Frozen semantic approvals and stable decision identifiers remain historical
authority and are not renamed or rewritten as evidence of new verification.

`@system` is a required current language feature, never a retired source API.
The user explicitly requires its verified C++/Verilog simulation closure before
this delivery can merge. Do not replace it with a module-only convenience path
or transfer compiler-generated clocks, resets, enables or testbench scaffolding
to the author. Preserve its semantic and independent-test responsibilities.

The current canonical source decorators are bare `@system`, `@rule`, and
`@module`, imported from `pycircuit`. Do not introduce another prefixed or
versioned frontend. On the final 2026-10-07 time cutoff, the user explicitly
stopped further test/example adaptation and requested a PR with remaining work
listed honestly. No full 93-root adaptation or nightly pass may be claimed.
