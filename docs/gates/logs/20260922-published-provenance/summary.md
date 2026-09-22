# Canonical source provenance through AC unit publication

## Gap

The frontend carries source positions as MLIR debug locations (`loc("file.py":9:11)`).
The native `acir-verify` stage published a unit with default printer flags, so
every location disappeared from the published text, and queue-graph planning only
reads the canonical `ac.source_provenance` attribute. A package therefore lost the
placement provenance of every `ac.instance`, and the closure capture reported the
entry file's post-flattening position rather than the defining file's position.

Bisect on the three-file fixture:

```text
frontend lower_queue_source          loc(...) = 2   ac.source_provenance = 0
native acir-verify publish (before)  loc(...) = 0   ac.source_provenance = 0
native acir-verify publish (after)   loc(...) = 0   ac.source_provenance > 0
```

## Change

- `materializeSourceProvenance` is now a shared transform entry point and runs in
  the Driver's `AcirVerify` publish path, so a published unit carries the
  canonical attribute for every operation that has a location. Operations that
  already carry it are untouched, so the freeze stage and publication agree.
- `_capture_worker._flatten_source_closure` captures each definition's original
  node locations with the existing `capture_source_node_locations` helper and
  forwards them as `source_node_locations`, so the reparsed flattened text keeps
  the real file, line, and column of every statement.

## Evidence

`test_multi_unit_package.py` (5 passed) now pins:

- the bundle's `module_instances` provenance exactly —
  `source/core.py:9:11` (`child_a(x)`) and `source/core.py:10:11` (`child_b(mid)`)
  for the fixture, i.e. the original call sites;
- that a published source unit carries `ac.source_provenance` and no MLIR
  locations, with a frame pointing at the original rule-body line;
- item V05's **module** construct as a checked-in source-map golden,
  `tests/goldens/agentic-circuit/source-map/module.json` (537 B, schema-validated,
  non-vacuous: any line change breaks the comparison).

| lane | result |
| --- | --- |
| `tests/python/agentic-circuit` (pytest, ignore tools/) | 494 passed, 1 skipped, 5 pre-existing failures |
| `tests/unit -m unit` | 217 passed |
| `check-acir` | 258 passed, 13 failed (all `verilator: command not found`) |
| `ctest` | 6/6 passed |

The five failures are the same pre-existing ones (2 environmental
`cli/test_discovery_commands.py`, 3 in the untracked WIP test). Reverting the
capture location change makes the two provenance assertions fail, so the fix is
load-bearing.

## Scope

Publication provenance and capture fidelity only. Item V05's **specialization**
construct still needs a package that carries a parameterized family across units;
the module construct is covered here.
