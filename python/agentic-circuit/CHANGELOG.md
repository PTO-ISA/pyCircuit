# Changelog

All notable changes to Agentic Circuit will be documented here. The project
follows Keep a Changelog and will use Semantic Versioning after its first
release.

## Unreleased

### Added

- Reproducible LLVM/MLIR 22.1.8 repository and development-toolchain baseline.
- A strict repository-local DavinciOO JSONL adapter that emits canonical,
  validated `pto-trace@0.1` documents without widening the simulator input
  contract.
- Committed runtime statistics and Chrome Trace Event JSONL, plus a
  deterministic repository-local Perfetto packer.
- Six complete golden workspaces covering queueing,
  backpressure, request/response memory, nested arrays, time-domain bridging,
  and suspended processes.
- A hierarchical superscalar NPU showcase with typed decode, dependency-aware
  oldest-ready issue, four finite execution-engine classes, memory behavior,
  completion, and in-order retirement.
- Workspace replay, equivalent-root, legal Work-permutation, dependency-scan,
  installation, sanitizer, and end-to-end CI gates.
- A provenance-locked DavinciOO gfsim C++ reference model under
  `third_party/references`, with an executable smoke trace and an explicit
  ACIR-to-gfsim generation boundary.
