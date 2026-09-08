# Circuit Flow Viewer

Standalone offline Queue/Table visualization. This package does not import
pyCircuit and needs no browser server or CDN. Its source is tracked under `third_party/circuit-flow-viewer` as an independent
package; it is not imported or bundled by the framework runtime.

```sh
python -m pip install --no-deps --no-build-isolation .
circuit-flow-viewer render execution.pyctrace --output replay.html
```

For development without installation:

```sh
PYTHONPATH=src python -m circuit_flow_viewer.cli render execution.pyctrace --output replay.html
PYTHONPATH=src python -m unittest discover -s tests -v
```

Open the resulting self-contained HTML in a browser. Queue cells are logical
FIFO positions (not physical addresses); Table rows retain their index. Click
cells/actions for exact values, drag card headings, and use play, step, seek,
and zoom controls. Read/write stages are visual explanations of an atomic
commit, not extra simulated cycles. Connections show participation in one
operation, not a field-level dependency or identity across transformed values.

Version 0.1 supports one-dimensional tables, scalar values, and flat records.
Nested entries are explicitly marked unsupported. No sampling or truncation is
performed. A truncated input can show only its last complete commit; corruption
is reported in the page. Large traces currently load into memory.

## Browser verification

Development-only dependency: Playwright 1.58.2 and Chromium. No browser code or
dependencies are required by pyCircuit.

```sh
node tests/browser.mjs /path/to/playwright replay.html evidence
```

The test checks navigation, atomic animation, precise reconstructed states,
local-only loading, table/queue rendering, dragging, and screenshots.

Queue display uses adjacent 24 × 24 px cells, with the name on the left. Empty cells are white; occupied cells are colored; waiting entries use stripes. Cells contain no visible payload text. Click a cell to inspect its current logical FIFO position in the side panel; selection follows that position across commits. Drag the name to move a Queue.

A flat Table with exactly one entry and one field is displayed as a register: its name beside a value box. Click for details; read/write highlights still use the recorded Table events. Multi-row or multi-field Tables retain their tabular display.
