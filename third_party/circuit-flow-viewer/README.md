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

Version 0.1 supports one-dimensional tables, scalar values, and recursive record
and array values. Tables use collapsible, grouped headers for nested fields;
selection preserves the exact integer value and declared width. Enum values
retain their encoded integers and widths. No sampling or truncation is
performed. A truncated input can show only its last complete commit; corruption
is reported in the page. Large traces currently load into memory.

## Browser verification

Development-only dependency: Playwright 1.58.2 and Chromium. No browser code or
dependencies are required by pyCircuit.

```sh
node tests/browser.mjs /path/to/playwright replay.html evidence
node tests/browser_nested.mjs /path/to/playwright nested-replay.html evidence
```

The test checks navigation, atomic animation, precise reconstructed states,
local-only loading, table/queue rendering, dragging, and screenshots.

Queue display uses adjacent 24 × 24 px cells, with the name on the left. Empty cells are white; occupied cells are colored; waiting entries use stripes. Cells contain no visible payload text. Click a cell to inspect its current logical FIFO position in the side panel; selection follows that position across commits. Drag the name to move a Queue.

A flat Table with exactly one entry and one field is displayed as a register: its name beside a value box. Click for details; read/write highlights still use the recorded Table events. Multi-row or multi-field Tables retain their tabular display.

### Source labels

When descriptors provide `display_name`, `display_path` and
`display_parent_path`, cards use those labels and group by their display parent.
Otherwise names are shown unchanged, without decoding compiler symbols.
Click a card heading to inspect its canonical identity and module parameters.
Run `tests/browser_source_names.mjs` with the same Playwright, HTML and evidence
arguments as the nested-record browser regression to validate ROB source labels.

### Table fields and rows

Nested structures start collapsed. Click a header arrow to expand one group,
or use **Expand all** / **Collapse all**. Group headers span their child columns;
leaf labels show only the local name. Hover for the complete field path.
Collapsed cells show `{…}` or an array length; click for exact typed details.

Each multi-row or multi-field Table provides:

- **Fields**: a checkbox tree with whole-group selection, mixed selection, select
  all and clear. Collapsing a group preserves its field selection.
- **Rows**: original row-index checkboxes, select all, clear, and ranges such as
  `0,1,4–7`. Invalid or out-of-bounds ranges leave the selection unchanged.
- A selected/total row count and explicit messages when no rows or fields are
  selected. Row indices are never renumbered by filtering.

All rows and fields start selected. Each Table maintains its own choices across
play, pause, step and seek; reloading the page restores defaults. Filtering does
not change recorded state, events, or atomic commit boundaries. Selection details
follow the same row and field at the current commit, even when hidden.

A collapsed group highlights when any descendant changes. A hidden-row access or
hidden-field change adds a **Show** action for the current visual phase; it reveals
the affected selection without advancing the replay. Fields are not automatically
filtered by domain-specific flags such as ROB `valid` or epoch values.

The viewer still loads the complete recording into memory; these controls reduce
visible cells, not trace size. No CLI or recording-format changes are required:
rerender an existing trace to obtain the new controls.

Run the generic typed-record and filtering regression with:

```sh
node tests/browser_table_tree.mjs /path/to/playwright rob-replay.html evidence
```

It builds a small independent two-Table fixture from the current HTML template,
checks exact integers, nested arrays, selection isolation and atomic navigation,
then checks filtered navigation on the supplied real ROB page.
