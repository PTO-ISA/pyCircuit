# Agentic Circuit Specifications

This directory contains the human-readable contracts for Agentic Circuit.
Machine-readable schemas under `schemas` and MLIR ODS
definitions under `include/acir` remain the executable
sources of truth.

The generated official Queue building-block catalog
records the closed opcode roles, arity, constants, backend realizations, and
refinement observations.

## Current manuals

- [Agentic Circuit Specification Manual](agentic-circuit.md) defines the
  implementation-facing serial Python, ACIR, gfsim, PYC, and refinement
  contract.
- [Agentic Circuit 团队 Specification 手册](agentic-circuit.zh-CN.md)
  provides a Chinese teammate-facing overview, common patterns, executable
  examples, backend differences, and troubleshooting guidance.

## NDF decision spine

The repository follows the restricted NDF profile declared in
[`ndf.yaml`](ndf.yaml). Stable clause IDs connect the current architecture,
decisions, historical evidence, and verification:

- [`ARC-RELEASE-001`, `ARC-LAYOUT-001`, and `ARC-HISTORY-001`](00-charter/scope.md)
  define release ownership, semantic source partitions, and history policy.
- [`D-RELEASE-LAYOUT-001`](../../rfcs/acir/D-RELEASE-LAYOUT-001.md) records the
  hard-break release-neutral layout decision.
- [`D-BLOCK-MODEL-001`](../../rfcs/acir/D-BLOCK-MODEL-001.md) records the current
  Queue/Var building-block model.
- [`D-RULE-LOWERING-001`](../../rfcs/acir/D-RULE-LOWERING-001.md) records the
  simple Python rule surface, staged MLIR lowering, and typed-marker contract
  for incomplete inference.
- [`REF-HISTORY-001`](refs/history.md) pins removed historical specifications
  to an immutable Git revision.
- [Repository contract verification](verification.md) binds release, layout,
  and history requirements to executable checks.
- [`VER-LAYOUT-001`](../../development/acir/verification/repository-layout.md) binds these rules to
  executable repository checks.
- [`VER-PYC-VERILOG-001`](../../development/acir/verification/pyc-verilog-backend.md) records the
  integrated PYC-to-Verilog bridge and its executable fixtures.
- [IR coverage ledger](../../development/acir/verification/ir-coverage.md) is generated from the
  current ACIR/ACSim manifests and lit coverage.

Product releases are represented by Git tags and GitHub Releases. The source
tree does not retain product-version or implementation-phase paths, aliases,
or compatibility symlinks. Serialized contract epochs and external dependency
pins remain versioned where interoperability and reproducibility require them.
