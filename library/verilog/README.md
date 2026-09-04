# pyCircuit Verilog runtime

This directory is the canonical cycle-level RTL library for the pyCircuit 6
hardware path.  It contains the original `pyc_*` primitives and the reviewed
`pyc_runtime_*` adapters imported from the Agentic Circuit runtime campaign.
Every adapter exposes packed, stable Verilog ports; callers do not need to
depend on an upstream SystemVerilog interface or typedef.

## Inventory and provenance

- `catalog.json` contains the 132 accepted components, their canonical
  parameters/ports, semantic oracle, dependency closure, provenance, and
  Verilator/Yosys status.
- `catalog.lock.json` pins the upstream revisions used by the vendored source.
- `manifests/` contains the parameter sweep and SHA-256 records for each
  promotion batch (the latest release is `parameterized-components-v0.30.json`).
- `licenses/` contains the license text for each external closure.
- `vendor-v*/` contains only the source/header/package files required by an
  accepted wrapper.  Candidate caches are intentionally not part of the
  runtime package.

The runtime catalogue is self-contained: all paths in `catalog.json` are
relative to this directory.  The four historical in-tree modules
(`pyc_fifo.v`, `pyc_popcount.v`, `pyc_reg.v`, and `pyc_rr_arbiter.v`) remain the
PTO-ISA versions already used by the ACIR Verilog tests; their thin
`pyc_runtime_*` adapters are included alongside the external wrappers.

## Crawler and validation

The reusable crawler/validation tools live in `tools/runtime/`.  Configurations
under `crawler/` are deterministic and remote sources are disabled by default.
The checked-in source manifest can scan this package without downloading code:

```bash
python tools/runtime/acir_runtime.py scan \
  --config library/verilog/crawler/sources.json \
  --output .pycircuit_out/runtime-crawl
```

Verify the package and its dependency closure first:

```bash
python tools/runtime/acir_runtime.py verify-catalog \
  --catalog library/verilog/catalog.json
python tools/runtime/acir_runtime.py vendor-check \
  --catalog library/verilog/catalog.json \
  --manifest library/verilog/manifests/parameterized-components-v0.30.json \
  --report .pycircuit_out/runtime-vendoring-check/v30.json
```

The packaged Verilator/Yosys gate is serial by default to keep memory bounded:

```bash
python tools/runtime/acir_runtime.py verify-runtime \
  --catalog library/verilog/catalog.json \
  --report .pycircuit_out/runtime-catalog-validation/report.json \
  --verilator wsl:verilator --yosys wsl:yosys --timeout 45
```

Functional oracles use the same canonical wrapper contract.  Power is not
claimed by the structural gate; the manifests record cell/wire QoR and power
requires a target Liberty and activity trace.
