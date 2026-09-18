# Changelog

All notable changes to Agentic Circuit will be documented here. The project
follows Keep a Changelog and will use Semantic Versioning after its first
release.

## Unreleased

### Added

- `acc.py -c <module.py> -o <module.ac>` and native `acc` single-TU C++,
    multi-TU C++, and Verilog emission, with source-named module files, stable
  parameter-derived specialization names, NDF comments, and Python source traceability
  (Decision 0266).

- Removed the retired `compile`, `build`, and `model` commands, persistent JIT
  caches, build inventories, and content-derived Python identities. Source
  closure is now captured once in memory for each compilation.

- Scalar lexical register reset images and write enables on `ac.var`, plus
  per-module `.ac.mlir` dumps, nominal gfsim type headers, and real out-of-line
  per-specialization `.h`/`.cpp` units compiled separately and linked from the
  root QueueGraph model (Decision 0264).
- Reproducible LLVM/MLIR 22.1.8 repository and development-toolchain baseline.
- Committed runtime statistics and Chrome Trace Event JSONL, plus a
  deterministic repository-local Perfetto packer.
- Vendor-neutral Queue, state, backpressure, time-domain, and process runtime
  regressions.
