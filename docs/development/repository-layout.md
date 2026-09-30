# Repository layout

The active framework owns the Python capture/driver, native source compiler,
common hardware representation, C++/Verilog emitters, one generated-model
Runtime, vendor-neutral examples, and framework tests. Product-specific
architectural models, ISA catalogs, consumer testbenches, reference models,
and model-comparison adapters belong in their consumer repositories.

The current public workflow and support boundary are documented in the
[M5 migration guide](m5-migration.md). The source-owned example at
`examples/pycircuit/counter/` demonstrates the current build graph. Older
`python/agentic-circuit`, QueueGraph, PYC emitter, CycleAwareSignal, and
sidecar paths shown in historical repository maps are retired and do not define
current package ownership.

## Active build ownership

- `python/pycircuit/`: source capture and public compile/link/emit driver.
- `compiler/acir/`: native compiler/MLIR implementation backing the approved
  Pythonic source route; ACIR naming in the internal tree is an implementation
  detail, not an old `agentic_circuit` public frontend.
- `simulator/gfsim/`: the single generated-model Runtime.
- `examples/pycircuit/counter/`: source-owned integration example.
- `tests/`, `flows/`: implementation and candidate validation assets, subject
  to the active migration inventory.

This summary is intentionally limited to active product roles. Use Git history
and the historical development ledgers for prior layouts.
