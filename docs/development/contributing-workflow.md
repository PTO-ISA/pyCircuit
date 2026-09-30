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

## Active interface boundaries

The M5 profile uses portless function `@module`, nested `@rule`, one default
clock, and empty static arguments. Do not add an unapproved Python/CLI/IR
contract, external ports, queue semantics, a full `@system` runner, memory/CDC,
multi-clock, or four-state source behavior. Unsupported uses must fail clearly;
retired frontends are not fallback implementations.

Runtime-only consumers should be able to use the exported
`pycircuit::pyc6_runtime` target without LLVM. CompilerDev is tied to LLVM/MLIR
22.1.8. Keep those package profiles independent.

## Documentation and examples

Update active user docs when source behavior, commands, output ownership,
installation, or gate expectations change. Treat examples as product claims:
only label a source as supported after compiling it through the active
compile/link/emit path and recording its evidence. Legacy callers awaiting
migration must be identified as such, not silently presented as current
examples.

## Review and merge

Review semantic behavior, rejection cases, both backend outputs, package
boundaries, and build/install references before style. Do not claim full M5
acceptance until its cutover candidate has independent tests, reviews, and
acceptance evidence. No compatibility aliases or forwarding targets should be
added to ease migration.
