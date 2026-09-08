# Table tree and selection evidence

Decision 0228 presentation follow-up. The standalone viewer now renders nested
Table fields under collapsible grouped headers, with independent field and
original-row selection. Top-level structures start collapsed, all rows/fields
selected. Per-Table choices persist through navigation and reset on page reload.
No public API, CLI, trace encoding, compiler, runtime or ROB behavior changed.

## Results

- Viewer reader/render unit tests: 4 passed (`unit.log`).
- Generic browser fixture and real filtered ROB projections: passed for all five
  delivered pages (`tree-*.log`). The independent two-Table fixture covers nested
  records/arrays, mixed checkboxes, invalid ranges, empty selections, exact u64
  and enum widths, collapsed/hidden changes, selection isolation, keyboard
  expansion, pause/step/seek, and atomic state updates. Every real trace commit
  projection was checked with rows and fields hidden.
- Existing nested ROB, source-name and legacy Queue/Table browser gates passed
  (`single-recovery.log`, `dual-recovery.log`, `legacy.log`). No page errors or
  external resource requests. Playback and delayed Queue behavior remain covered.
- Scoped pre-commit checks, strict MkDocs build and diff whitespace checks passed
  (`format.log`, `docs.log`).

## Artifacts and reproduction

Use `commands.md` and `regenerate.py`. `artifacts.json` records source trace paths
and exact input/output hashes. The entry point is
`.pycircuit_out/davincioo-rob/20260908-table-tree/index.html`.
Four single-ROB scenarios and dual-ROB recovery were rerendered from existing
functionally verified traces; this run did not rebuild or rerun native models.
Original IR, gfsim binaries and drivers remain in their source run directories.
Screenshots are under the output directory's `browser/` subdirectories.

## Issues resolved during validation

- Chromium cannot launch in the restricted sandbox (`shutdown: EPERM`); browser
  gates ran with approved escalation against local file URLs.
- The first test attempted to uncheck an already half-selected checkbox using
  Playwright's checked-state API, which performs no click in that state. The test
  now explicitly selects then clears the group; mixed-state UI behavior passes.
- The evidence script's import ordering/output style was corrected for Ruff.
- A source-tree README link was outside MkDocs' documentation set. The guide now
  names its checkout path; strict documentation validation passes.

## Limits

Filtering reduces visible cells, not loaded trace memory. Choices are local to
one open page; no persistence/export or ROB-specific validity inference is added.
No simulator gates were rerun because only presentation and its tests changed;
the original single-ROB and source-names gate directories retain native evidence.
Decision completion claims and trace/reset contracts did not change, so this
presentation follow-up does not rerun full decision-status or G2 release lanes.
