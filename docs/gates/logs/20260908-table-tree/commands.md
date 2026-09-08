# Table tree replay reproduction

Run from `/home/lc/pyCircuit`. This is a Decision 0228 presentation-only gate;
no runtime, compiler or recording-format semantics changed.

```sh
RUN=/home/lc/.codex/skills/pyc6/scripts/run.sh
$RUN env PYTHONPATH=third_party/circuit-flow-viewer/src python docs/gates/logs/20260908-table-tree/regenerate.py
$RUN env PYTHONPATH=third_party/circuit-flow-viewer/src python -m unittest discover -s third_party/circuit-flow-viewer/tests -v
```

`regenerate.py` records exact source trace and output HTML hashes in
`artifacts.json`. Original compiled models and native functional-test evidence
remain in the source run directories; no simulation was rerun for presentation.

Run the browser commands below outside the restricted sandbox: Chromium's
sandbox-host shutdown syscall is denied inside it. All pages load through
`file:` without a server or network resources.

```sh
export LD_LIBRARY_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-deps/usr/lib64
export PLAYWRIGHT_BROWSERS_PATH=/home/lc/pyCircuit/.pycircuit_out/replay/browser-cache
RUN=/home/lc/.codex/skills/pyc6/scripts/run.sh
P=.pycircuit_out/replay/browser/node_modules/playwright
O=.pycircuit_out/davincioo-rob/20260908-table-tree
for scenario in single-capacity single-backpressure single-invalid single-recovery dual-recovery; do
  $RUN node third_party/circuit-flow-viewer/tests/browser_table_tree.mjs "$P" "$O/$scenario/replay.html" "$O/browser/tree-$scenario"
done
$RUN node third_party/circuit-flow-viewer/tests/browser_nested.mjs "$P" "$O/single-recovery/replay.html" "$O/browser/single-recovery"
$RUN node third_party/circuit-flow-viewer/tests/browser_source_names.mjs "$P" "$O/dual-recovery/replay.html" "$O/browser/dual-recovery"
$RUN node third_party/circuit-flow-viewer/tests/browser.mjs "$P" "$O/legacy/replay.html" "$O/browser/legacy"
$RUN pre-commit run --files third_party/circuit-flow-viewer/src/circuit_flow_viewer/viewer.html third_party/circuit-flow-viewer/tests/browser_table_tree.mjs third_party/circuit-flow-viewer/tests/browser_nested.mjs third_party/circuit-flow-viewer/README.md docs/development/acir/queue-table-flow.md docs/gates/logs/20260908-table-tree/regenerate.py docs/gates/logs/20260908-table-tree/commands.md docs/gates/logs/20260908-table-tree/summary.md
$RUN python -m mkdocs build --strict --site-dir .pycircuit_out/table-tree-site
git diff --check
```
