# Retired ACIR-to-PYC/Verilog verification path

This verification document belongs to the retired Agentic Circuit, QueueGraph,
PYC, and `pycc` pipeline. Its commands and evidence do not validate the active
M5 source-unit candidate.

The current route emits Verilog from the same linked final design used by C++:
`pycircuit compile` → `pycircuit link` → `pycircuit emit --target verilog`.
The candidate still requires source-map, output-preservation, installed
consumer, and backend acceptance evidence. See the
[M5 migration guide](../../m5-migration.md).
