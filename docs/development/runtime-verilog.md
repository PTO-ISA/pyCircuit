# Runtime Verilog library and crawler

The integrated pyCircuit checkout owns the cycle-level runtime RTL and the
Agentic Circuit discovery flow.  New runtime work should be done in this
repository; the standalone Agentic Circuit checkout is historical input only.

## Repository layout

| Path | Role |
| --- | --- |
| `library/verilog/` | Canonical PYC RTL, `pyc_runtime_*` adapters, vendored source/header closures, licenses and catalog |
| `library/verilog/catalog.json` | 132 accepted components with canonical ports/parameters, oracle, provenance and gate status |
| `library/verilog/manifests/` | Parameter sweeps, tool versions and SHA-256 release records |
| `library/verilog/crawler/` | Runtime-local source/target/search manifests |
| `tools/pycircuit_rtl_crawler_v0.14/` | RTL crawler, frozen 355-candidate inventory and source materialization helpers |
| `tools/runtime/` | Runtime discovery, dependency closure, adapter, validation, promotion and vendoring commands |
| `examples/agentic-circuit/blocks/priority_encode_runtime/` | Python → ACIR → PYC → Verilog example |
| `.pycircuit_out/` | Disposable crawl, build, gate and demo outputs |

The runtime package deliberately contains the accepted dependency closure, not
the large candidate cache.  The crawler can recreate external source caches
from the pinned manifests and the frozen inventory remains in
`tools/pycircuit_rtl_crawler_v0.14/inventory/`.

## Package checks

Run these from the repository root:

```bash
python tools/runtime/acir_runtime.py verify-catalog \
  --catalog library/verilog/catalog.json
python tools/runtime/acir_runtime.py vendor-check \
  --catalog library/verilog/catalog.json \
  --manifest library/verilog/manifests/parameterized-components-v0.30.json \
  --report .pycircuit_out/runtime-vendoring-check/v30.json
python tools/runtime/acir_runtime.py verify-runtime \
  --catalog library/verilog/catalog.json \
  --report .pycircuit_out/runtime-catalog-validation/report.json \
  --verilator wsl:verilator --yosys wsl:yosys --timeout 45
```

The last command is serial and bounded per entry.  It checks the packaged
wrapper plus its complete closure; it does not claim dynamic power.  The
manifest records structural cell/wire QoR, while power requires a Liberty and
activity trace supplied by a later technology-specific flow.

## Discovery and promotion

The crawler source manifest is `library/verilog/crawler/sources.json`.  Remote
repositories are disabled by default.  A local-only scan is therefore safe:

```bash
python tools/runtime/acir_runtime.py scan \
  --config library/verilog/crawler/sources.json \
  --output .pycircuit_out/runtime-crawl
```

For the broader pyCircuit v0.14 inventory, use the copied crawler and its
materialized source cache.  The frozen list is
`tools/pycircuit_rtl_crawler_v0.14/inventory/candidates_frozen.csv`; full
validation is checkpointed and should be run with `--resume` and a bounded
`--tool-timeout` on constrained hosts.  Structural passes become candidates,
not accepted runtime APIs, until an adapter, semantic oracle, provenance and
license closure are present.  Promotion scripts in `tools/runtime/` preserve
this distinction.

## End-to-end example

The priority encoder example demonstrates a real parameterized flow:

```bash
python examples/agentic-circuit/blocks/priority_encode_runtime/run_demo.py
python examples/agentic-circuit/blocks/priority_encode_runtime/run_variants.py
```

The scripts use the integrated ACIR binaries under
`.pycircuit_out/acir/dev-llvm22/bin` (or `build/dev-llvm22/bin` as a fallback),
the runtime under `library/verilog/`, and write all generated ACIR, PYC IR,
Verilog and gate evidence under `.pycircuit_out/examples/`.
