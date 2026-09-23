# Instance specialization identity in the published plan

## What was missing

The QueueGraph plan already extracted each placement's `StaticArgumentsAttr`, but
neither plan printer published it, and the frontend rejected a second
specialization of one imported family:

```text
ACPY-FAMILY-008: one family symbol cannot carry an undeclared concrete body
```

A declaration without a local implementation registers a rule-module template, so
a second placement with different static arguments tripped the one-body-per-symbol
guard even though an import has no concrete case.

## Change

- The canonical plan JSON and the source map now publish
  `static_arguments: [{"name": ..., "value": <canonical attribute text>}]` on every
  module instance, which is the specialization identity of a placement.
- The collision guard now distinguishes a published import (no program) from a
  local concrete body: an imported family publishes exactly one
  `ac.module.import` and each placement keeps its own ordered static arguments.
- `schemas/agentic-circuit/source-map.schema.json` requires the new field, and the
  two package goldens are regenerated.

## Evidence

A package whose root unit places two specializations of one imported family
(`lanes = 2` and `lanes = 4`) now links and reports

```json
[{"name": "stage_0", "definition": "stage",
  "static_arguments": [{"name": "lanes", "value": "#ac.static_value<#ac.static_int_value<<4, false>, 2 : i4>>"}],
  "source_provenance": {"origins": [{"frames": [{"column": 13, "file": "source/core.py", "kind": "statement", "line": 13}]}]}},
 {"name": "stage_1", "definition": "stage",
  "static_arguments": [{"name": "lanes", "value": "#ac.static_value<#ac.static_int_value<<4, false>, 4 : i4>>"}],
  "source_provenance": {"origins": [{"frames": [{"column": 14, "file": "source/core.py", "kind": "statement", "line": 14}]}]}}]
```

`test_multi_unit_package.py` (6 passed) asserts both placements and their static
arguments, validates the map against the schema, and compares it with
`tests/goldens/agentic-circuit/source-map/specialization.json`.

| lane | result |
| --- | --- |
| `tests/python/agentic-circuit` (pytest, ignore tools/) | 495 passed, 1 skipped, 5 pre-existing failures |
| `tests/unit -m unit` | 217 passed |
| `check-acir` | 258 passed, 13 failed (all `verilator: command not found`) |
| `ctest` | 6/6 passed |

## Scope

Instancing identity and the cross-unit specialization rejection only. Issue #180
still needs a purpose-built manifest artifact with its own schema and a Python
API that returns the generated sources plus that manifest.
