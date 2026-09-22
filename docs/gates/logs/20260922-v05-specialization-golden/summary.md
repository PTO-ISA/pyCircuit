# Item V05: specialization source-map golden

## Coverage

Item V05 asks for source-map golden tests covering the helper, module,
projection, and specialization constructs. The first three are recorded in
`docs/gates/logs/20260922-v05-source-map-goldens/` (helper, projection) and
`docs/gates/logs/20260922-published-provenance/` (module). This run covers the
specialization construct, which needs a package that carries a parameterized
family across units.

`test_multi_unit_package.py::test_specialized_family_source_map_matches_golden`
compiles a source unit that declares one parameterized family (`lanes` in
`{2, 4}`) with its implementation, a root unit that places one specialization
with `static=ac.case(("lanes", 2))`, links the package, and emits the structured
bundle. It then

- validates `share/generated/source-map.json` against
  `schemas/agentic-circuit/source-map.schema.json`,
- compares it byte for byte with `tests/goldens/agentic-circuit/source-map/specialization.json`
  (363 B), which records the placement `stage_0 -> @stage` with the call site,
- asserts the placement list contains exactly `stage`, and
- asserts the published unit keeps **both** declared cases with their static
  arguments (`2 : i4` and `4 : i4`), so the golden covers a specialization
  rather than a single concrete module.

A source unit is imported by the capture worker, so the fixture keeps its
dependent annotation lazy with `from __future__ import annotations`; the
specification now states that requirement.

## Result

```text
test_multi_unit_package.py: 6 passed
tests/python/agentic-circuit (pytest, ignore tools/): 495 passed, 1 skipped, 5 pre-existing failures
```

The goldens are non-vacuous: tampering with any line value in a golden fails the
byte comparison.

## Scope

Item V05 is complete: helper, module, projection, and specialization each have a
checked-in, schema-validated source-map golden, and the two hierarchical
constructs additionally assert the package properties they depend on. No other
matrix item is claimed here.
