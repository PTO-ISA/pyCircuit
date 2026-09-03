# DavinciOO goldens

- `softmax-projection.json` defines the committed behavioral projection used by
  gfsim and PYC refinement tests.
- `davincioo-softmax-run.json` is the canonical generated run report.
- `davincioo-softmax-swimlane.svg` is the canonical trace visualization.

Regenerate the run and visualization with `tools/run-davincioo.py`; tests reject
any byte-level drift.
