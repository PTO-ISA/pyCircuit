# Changelog

All notable changes to Agentic Circuit will be documented here. The project
follows Keep a Changelog and will use Semantic Versioning after its first
release.

## Unreleased

### Added

- Scalar lexical register reset images and write enables on `ac.var`, plus
  per-module `.ac.mlir` dumps, nominal gfsim type headers, and real out-of-line
  per-specialization `.h`/`.cpp` units compiled separately and linked from the
  root QueueGraph model (Decision 0264).
- Reproducible LLVM/MLIR 22.1.8 repository and development-toolchain baseline.
- Committed runtime statistics and Chrome Trace Event JSONL, plus a
  deterministic repository-local Perfetto packer.
- Vendor-neutral Queue, state, backpressure, time-domain, and process runtime
  regressions.
