# pyCircuit Tools

| Tool | Purpose |
| --- | --- |
| `check-pyc-inventory.py` | Verify that PYC ODS, producers, coverage, and the generated inventory ledger agree |
| `generate-semantic-primitive-registry.py` | Validate the semantic primitive registry and emit its C++ table |
| `dump_pyctrace.py` | Inspect binary pyc6 traces with optional manifest decoding |
| `pyc_module_graph.py` | Extract and inspect the module/instance graph from PYC MLIR |
| `schematic_view.py` | Render a schematic from generated Verilog |
| `visualize_cpp.py` | Render a schematic from generated C++ model headers |

Run tools from the repository root. Use `--help` for command-specific options
and write generated output under `.pycircuit_out/`.
