# Architecture

pyCircuit has one source frontend and one common hardware representation:

```text
Python source → syntax capture → MLIR analysis and lowering
              → published source units → link and verification
              → final hardware IR → C++ / Verilog
```

Capture records syntax without executing design functions. MLIR resolves types,
interfaces, dependencies and effects. Behavioral variables and explicit standard
storage leaves both lower through the same hardware IR. Rules compute from old
state during Work; successful whole-system checks precede Xfer commit.

Each source owns its published interface and generated implementation group.
Parents consume interfaces, link receives the explicit complete closure, and
both backends consume the same saved final artifact. C++ modules use the shared
GFSIM Runtime; Verilog uses the standard storage library.

[Source semantics](../reference/language.md), [the compiler pipeline](compiler-pipeline.md)
and [runtime execution](simulation.md) describe the active contracts.
[Known limitations](../development/known-limitations.md) distinguishes missing
source features from narrower IR/runtime support.
