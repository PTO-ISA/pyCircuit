# Contributing workflow

Contributions target the single active pyCircuit source and compile route:
Python source units → verified final design → C++ or Verilog. The current
bounded profile and its unsupported capabilities are specified in the
[language reference](../reference/language.md).

## Standard change loop

1. Identify the public contract and affected decision records.
2. Keep the change within the approved source, IR, CLI, runtime, or build
   contract. If a contract is not approved, prepare a proposal rather than
   implementing a new public behavior.
3. Update behavior and workflow documentation with the implementation.
4. Run the narrowest relevant check, followed by the required candidate gates.
5. Save reviewable evidence under `docs/gates/logs/<run-id>/` when behavior,
   compile flow, package ownership, or a release claim changes.
6. Report changed files, commands and outcomes, candidate identity, and known
   limits in the review description.

Build and validate from the current checkout. Do not copy compilers, libraries,
or generated artifacts from another worktree. Keep temporary scripts and
outputs under tests, examples, docs, or disposable `.pycircuit_out/` paths.

## Trace a change through its owners

Before implementation, map the proposed behavior to the existing owners below.
Record only affected layers and explain a boundary that might otherwise look
incomplete. This is an impact checklist, not a requirement to edit every layer.

| Layer | What to check |
| --- | --- |
| Python capture | Syntax/transport and source provenance; no execution or duplicate semantic inference |
| MLIR analyses and common IR | Types, dependencies, effects, ownership, verification and lowering; accepted and rejected forms |
| Publication and link | Body/interface agreement, explicit provider closure and failed-output protection |
| C++ and Verilog | Same verified final artifact, layout/four-state behavior and matching operation support |
| Runtime and System | Work/commit/failure behavior and generated `@system` execution when affected |
| CLI, build and package | Diagnostics, installed inventory, Runtime-only consumers and exact CompilerDev dependency |
| Tests and documentation | Independent oracles, current supported behavior, unsupported cases and removed callers |

For a pass failure, retain the smallest reproducer, exact pass/stage command,
input IR, expected invariant and actual output or diagnostic. Identify the first
stage that diverges before changing an emitter. Reuse the existing stage-output
options and manifests described in the [source-unit workflow](source-unit-workflow.md).
Unavailable artifacts stay explicitly unavailable; neither an excerpt nor a
hand-written expected IR establishes a successful compilation.

For a performance claim, compare explicit baseline and candidate revisions with
identical workload bytes, parameters, toolchain/build mode and worker count.
Build each revision in its own checkout. Record measurement commands, units,
warmup/repetition policy and variation, alongside correctness checks. Separate
capture/compile/link/emit time, generated code size, simulation time and memory
when they explain the result. Investigate repeated full scans, nested traversals
and cache lifetimes without weakening validation budgets or input coverage.

When comparing generated artifacts, enumerate added, removed, changed, unchanged
and missing outputs using the existing manifests. Compare source/typed stages,
final IR and both targets where available. Text differences locate a regression;
they do not prove semantic equivalence, performance improvement or complete
coverage. Long comparison matrices still belong in the existing nightly tier.

## Active interface boundaries

The current module profile uses typed values, Module-owned variables and
source-global or nested `@rule` functions. ACIR infers storage, control and
transaction lowering; Work/Xfer belongs to generated execution. See the language reference for supported
source expressions and the typed C++ DUT. Closed default-domain `@system`
simulation uses generated C++ and Verilator/Icarus closures; explicit X/Z
scenarios require the four-state Icarus path. Broader system forms, source collection authoring,
general arithmetic and automatic multi-clock scheduling remain unfinished.
Unsupported uses must fail clearly; retired frontends are not fallback
implementations. Existing user authorization remains valid for its scope.

Runtime-only consumers should be able to use the exported
`pycircuit::pyc6_runtime` target without LLVM. CompilerDev is tied to LLVM/MLIR
22.1.8. Keep those package profiles independent. CompilerDev installs the active
Compiler header closure explicitly; retained migration references are not SDK
interfaces. When an active header adds a dependency, update the install list and
verify it through an installed consumer.

## Documentation and examples

Update active user docs when source behavior, commands, output ownership,
installation, or gate expectations change. Treat examples as product claims:
only label a source as supported after compiling it through the active
compile/link/emit path and recording its evidence. Legacy callers awaiting
migration must be identified as such, not silently presented as current
examples.

## Review and merge

Review semantic behavior, rejection cases, both backend outputs, package
boundaries, and build/install references before style. Accept only the tested
candidate with the applicable independent tests, reviews and evidence. Do not
add compatibility aliases or forwarding targets that bypass current contracts.

## External reference assets

Keep an imported comparison model or source snapshot with its owning test,
not in an empty repository-wide placeholder. Record its upstream repository,
exact commit, selected files and license in the fixture's README or manifest.
Reference code must remain independent of DUT lowering and generated results.
An external snapshot is not installed product code unless its actual build and
package owner explicitly includes it. Preserve that distinction when reviewing
new dependencies or moving test assets.
