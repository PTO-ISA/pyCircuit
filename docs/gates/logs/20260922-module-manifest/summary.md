# Issue #180: the module manifest artifact

## What was missing

The bundle published the emitted cost report and the source map, but no manifest
of module definitions and placements, so a consumer had to recover the
definition/case inventory from generated C++ names.

## Change

A structured bundle now publishes `share/generated/module-manifest.json`
(`agentic-circuit-module-manifest`, version 0.1), derived from the typed plan
(`QueueGraphPlan::moduleManifestJson`), with

- `modules`: one entry per family — `symbol`, `owner`, `declaration`, `interface`
  ports (name, direction, logical type), `parameters` (name, required, type),
  `declared_cases` (the declared static-argument inventory), and `cases` (the
  concrete case signatures with their static arguments);
- `instances`: one entry per placement — `definition`, `name`, `scope`, the
  ordered typed `static_arguments` that select the case, and `source_provenance`.

`schemas/agentic-circuit/module-manifest.schema.json` publishes the contract and
is checked by the repository contract gate. Two native tests that pin the bundle
inventory and its indexed artifact order were updated for the new file.

## Evidence

For a package whose root unit places two specializations of one imported family,
`test_multi_unit_package.py` (6 passed) validates the manifest against the schema,
compares it byte for byte with `tests/goldens/agentic-circuit/module-manifest/specialization.json`
(2739 B), and asserts the family's parameters, declared cases, and concrete case
signatures. The linked hierarchy test asserts the manifest's module symbols,
placement definitions, and inputs-before-outputs port order.

```text
CodeGenTests --gtest_filter='*QueueGraphPlan*': 110 passed
tests/python/agentic-circuit (pytest, ignore tools/): 495 passed, 1 skipped, 5 pre-existing failures
tests/unit -m unit: 217 passed
check-acir: 258 passed, 13 failed (all verilator: command not found)
ctest: 6/6 passed
check-contracts.py: OK (public schemas)
```

## Scope

The manifest artifact only. Issue #180 still needs a Python caller that returns
the generated sources plus that manifest; today a consumer drives `acc.py` and the
native `acc` itself.
